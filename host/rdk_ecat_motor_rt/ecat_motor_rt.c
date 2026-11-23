/**
 * @file ecat_motor_rt.c
 * @brief 16 轴 HPM 网关的单线程周期 EtherCAT 主站。
 * @author BearKing
 *
 * SOEM 的原始以太网套接字一次只允许一笔过程数据事务在途；本文件将所有
 * ecx_* 调用限制在同一个周期线程中。
 */

#define _POSIX_C_SOURCE 200809L

#include "motor_pdo.h"

#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/mman.h>
#include <sched.h>
#include <unistd.h>

#include "soem/soem.h"

#define GATEWAY_RT_IOMAP_SIZE_BYTES          4096U
#define GATEWAY_RT_DEFAULT_CYCLE_TIME_US     1000U
#define GATEWAY_RT_DEFAULT_RX_TIMEOUT_US      500U
#define GATEWAY_RT_MAX_CYCLE_TIME_US       100000U
#define GATEWAY_RT_MAX_SCHED_PRIORITY          99

typedef struct {
    const char *interface_name;
    uint32_t cycle_us;
    uint32_t rx_timeout_us;
    int rt_priority;
    bool arm_all_axes;
    bool print_feedback;
} gateway_rt_options;

typedef struct {
    atomic_uint_fast64_t cycles;
    atomic_uint_fast64_t wkc_errors;
    atomic_uint_fast64_t deadline_misses;
    atomic_uint_fast64_t max_exchange_us;
    atomic_uint_fast64_t max_lateness_us;
} gateway_rt_runtime_stats;

typedef struct {
    /**< SOEM 上下文、原始套接字及全部 ecx_* 调用均由周期线程独占。 */
    ecx_contextt soem;
    bool soem_initialized;
    int slave_index;
    int expected_wkc;
    /**< SOEM 在配置阶段写入此过程映像。 */
    uint8_t iomap[GATEWAY_RT_IOMAP_SIZE_BYTES];
    /**< 应用拥有的命令映像；每个周期复制到 slave->outputs。 */
    gateway_rt_output_pdo output;
    gateway_rt_runtime_stats stats;
} gateway_rt_master;

/** 异步信号处理函数与周期线程之间唯一共享的状态。 */
static volatile sig_atomic_t s_stop_requested;

/// @brief 记录终止请求，使周期线程在周期边界退出。
/// @param signal_number POSIX 信号编号；仅用于满足信号处理函数签名。
static void request_stop(int signal_number)
{
    (void)signal_number;
    s_stop_requested = 1;
}

/// @brief 输出命令行用法、时序参数和电机使能确认要求。
/// @param program 当前可执行文件名。
static void usage(const char *program)
{
    fprintf(stderr,
            "Usage: %s <interface> --arm-all [--cycle-us N] [--rx-timeout-us N] [--rt-priority N]\n"
            "\n"
            "  --arm-all          Required acknowledgement: current gateway firmware auto-enables\n"
            "                     every motor enabled in motor_config.c after its first\n"
            "                     command PDO.\n"
            "  --cycle-us N       EtherCAT period in microseconds (default: %u).\n"
            "  --rx-timeout-us N  SOEM receive timeout in microseconds (default: %u).\n"
            "  --rt-priority N    Optional SCHED_FIFO priority, 1..%d.\n"
            "  --print-feedback   Print IMU and all 16-axis feedback once per second.\n",
            program,
            GATEWAY_RT_DEFAULT_CYCLE_TIME_US,
            GATEWAY_RT_DEFAULT_RX_TIMEOUT_US,
            GATEWAY_RT_MAX_SCHED_PRIORITY);
}

/// @brief 解析一个无符号十进制命令行数值。
/// @param text 待解析的以 '\0' 结尾字符串，不能为空。
/// @param value 成功时写入解析结果的非空输出指针。
/// @return 输入合法且未溢出时返回 true，否则返回 false。
static bool parse_u32(const char *text, uint32_t *value)
{
    char *end = NULL;
    unsigned long parsed;

    errno = 0;
    parsed = strtoul(text, &end, 10);
    if ((errno != 0) || (end == text) || (*end != '\0') || (parsed > UINT32_MAX)) {
        return false;
    }

    *value = (uint32_t)parsed;
    return true;
}

/// @brief 校验全部命令行参数并生成受限的周期运行配置。
/// @param argc 命令行参数数量。
/// @param argv 命令行参数数组。
/// @param options 成功时写入配置的非空输出指针。
/// @return 参数完整、范围合法且明确确认 --arm-all 时返回 true。
static bool parse_options(int argc, char **argv, gateway_rt_options *options)
{
    int index;

    if ((argc < 2) || (options == NULL)) {
        return false;
    }

    memset(options, 0, sizeof(*options));
    options->interface_name = argv[1];
    options->cycle_us = GATEWAY_RT_DEFAULT_CYCLE_TIME_US;
    options->rx_timeout_us = GATEWAY_RT_DEFAULT_RX_TIMEOUT_US;

    for (index = 2; index < argc; index++) {
        uint32_t value;

        if (strcmp(argv[index], "--arm-all") == 0) {
            options->arm_all_axes = true;
        } else if (strcmp(argv[index], "--print-feedback") == 0) {
            options->print_feedback = true;
        } else if ((strcmp(argv[index], "--cycle-us") == 0) && ((index + 1) < argc) &&
                   parse_u32(argv[++index], &value)) {
            options->cycle_us = value;
        } else if ((strcmp(argv[index], "--rx-timeout-us") == 0) && ((index + 1) < argc) &&
                   parse_u32(argv[++index], &value)) {
            options->rx_timeout_us = value;
        } else if ((strcmp(argv[index], "--rt-priority") == 0) && ((index + 1) < argc) &&
                   parse_u32(argv[++index], &value) && (value <= GATEWAY_RT_MAX_SCHED_PRIORITY)) {
            options->rt_priority = (int)value;
        } else {
            return false;
        }
    }

    return options->arm_all_axes &&
           (options->cycle_us >= 500U) &&
           (options->cycle_us <= GATEWAY_RT_MAX_CYCLE_TIME_US) &&
           (options->rx_timeout_us > 0U) &&
           (options->rx_timeout_us < options->cycle_us);
}

/// @brief 原子地保留已观测到的最大值，不阻塞周期线程。
/// @param target 要更新的非空最大值原子变量。
/// @param value 本次观测值。
static void update_max_atomic(atomic_uint_fast64_t *target, uint64_t value)
{
    /* 统计保持无锁：最大值偏小可接受，丢失更大的观测值不可接受。 */
    uint_fast64_t observed = atomic_load_explicit(target, memory_order_relaxed);

    while ((value > observed) &&
           !atomic_compare_exchange_weak_explicit(target,
                                                  &observed,
                                                  value,
                                                  memory_order_relaxed,
                                                  memory_order_relaxed)) {
    }
}

/// @brief 读取单调时钟并转换为微秒。
/// @return 自单调时钟基准开始的微秒数，用于时长测量和统计。
static uint64_t monotonic_time_us(void)
{
    struct timespec now;

    (void)clock_gettime(CLOCK_MONOTONIC, &now);
    return ((uint64_t)now.tv_sec * UINT64_C(1000000)) + ((uint64_t)now.tv_nsec / 1000U);
}

/// @brief 向 timespec 添加微秒时长，并规范化纳秒进位。
/// @param time 原始时间点。
/// @param microseconds 要添加的时长，单位为微秒。
/// @return 相加并规范化后的时间点。
static struct timespec add_us(struct timespec time, uint32_t microseconds)
{
    time.tv_nsec += (long)(microseconds % 1000000U) * 1000L;
    time.tv_sec += (time_t)(microseconds / 1000000U);
    if (time.tv_nsec >= 1000000000L) {
        time.tv_sec++;
        time.tv_nsec -= 1000000000L;
    }
    return time;
}

/// @brief 执行可被停止信号中断的相对时长睡眠。
/// @param microseconds 睡眠时长，单位为微秒。
static void sleep_us(uint32_t microseconds)
{
    struct timespec delay;

    delay.tv_sec = (time_t)(microseconds / 1000000U);
    delay.tv_nsec = (long)(microseconds % 1000000U) * 1000L;
    while ((nanosleep(&delay, &delay) == -1) && (errno == EINTR) && !s_stop_requested) {
    }
}

/// @brief 按 EtherCAT 厂商标识和产品代码查找本项目的 HPM 网关。
/// @param context 已完成从站扫描的 SOEM 上下文。
/// @return 匹配从站的正索引；未找到时返回 -1。
static int find_gateway(const ecx_contextt *context)
{
    int slave;

    for (slave = 1; slave <= context->slavecount; slave++) {
        const ec_slavet *candidate = &context->slavelist[slave];

        if ((candidate->eep_man == GATEWAY_RT_VENDOR_ID) &&
            (candidate->eep_id == GATEWAY_RT_PRODUCT_CODE)) {
            return slave;
        }
    }

    return -1;
}

/// @brief 在给定超时内等待指定从站到达目标 EtherCAT 基础状态。
/// @param master 已初始化且包含有效从站索引的主站实例。
/// @param expected_state 目标 EtherCAT 基础状态。
/// @param timeout_us SOEM 状态检查超时，单位为微秒。
/// @return 从站低四位状态等于目标状态时返回 true。
static bool wait_for_state(gateway_rt_master *master, uint16_t expected_state, int timeout_us)
{
    uint16_t actual;

    actual = ecx_statecheck(&master->soem,
                            (uint16_t)master->slave_index,
                            expected_state,
                            timeout_us);
    /* EtherCAT 基础状态位于低四位；确认和错误标志不参与此比较。 */
    return (actual & 0x000FU) == expected_state;
}

/// @brief 发送有效的零过程数据并请求 OP 状态。
/// @param master 已映射 PDO 且包含目标从站的主站实例。
/// @return 网关在限定重试次数内进入 OP 时返回 true，否则返回 false。
/// @note 当前网关要求在 SAFE-OP 到 OP 前至少发送一帧有效输出 PDO。
static bool enter_operational(gateway_rt_master *master)
{
    ec_slavet *slave = &master->soem.slavelist[master->slave_index];
    int attempt;

    /* 当前网关要求 SAFE-OP 切换 OP 前存在一帧有效的输出 PDO。 */
    memset(slave->outputs, 0, GATEWAY_RT_OUTPUT_SIZE_BYTES);
    for (attempt = 0; attempt < 500; attempt++) {
        int wkc;

        (void)ecx_send_processdata(&master->soem);
        wkc = ecx_receive_processdata(&master->soem, EC_TIMEOUTRET);

        slave->state = EC_STATE_OPERATIONAL;
        ecx_writestate(&master->soem, (uint16_t)master->slave_index);
        if (wait_for_state(master, EC_STATE_OPERATIONAL, EC_TIMEOUTRET)) {
            printf("Gateway entered OP (initial WKC=%d, expected=%d)\n", wkc, master->expected_wkc);
            return true;
        }
        sleep_us(10000U);
    }

    fprintf(stderr,
            "Gateway did not enter OP: AL=0x%04X (%s)\n",
            slave->ALstatuscode,
            ec_ALstatuscode2string(slave->ALstatuscode));
    return false;
}

/// @brief 打开 SOEM、验证 HPM PDO 契约，并将匹配网关切换至 OP。
/// @param master 要初始化的主站实例。
/// @param options 已校验的运行配置，提供 EtherCAT 网卡名称。
/// @return 初始化、PDO 尺寸核验和 OP 切换全部成功时返回 true。
static bool configure_master(gateway_rt_master *master, const gateway_rt_options *options)
{
    ec_slavet *slave;

    if (!ecx_init(&master->soem, options->interface_name)) {
        fprintf(stderr, "ecx_init(%s) failed\n", options->interface_name);
        return false;
    }
    master->soem_initialized = true;
    if (ecx_config_init(&master->soem) <= 0) {
        fprintf(stderr, "No EtherCAT slaves found\n");
        return false;
    }

    master->slave_index = find_gateway(&master->soem);
    if (master->slave_index < 0) {
        fprintf(stderr, "HPM gateway (vendor=0x%08X product=0x%08X) not found\n",
                GATEWAY_RT_VENDOR_ID,
                GATEWAY_RT_PRODUCT_CODE);
        return false;
    }

    /* PDO 仅在此映射一次；周期路径只复制定长映像，绝不动态分配内存。 */
    (void)ecx_config_map_group(&master->soem, master->iomap, 0U);
    ecx_configdc(&master->soem);
    slave = &master->soem.slavelist[master->slave_index];

    if ((slave->Obytes != GATEWAY_RT_OUTPUT_SIZE_BYTES) ||
        (slave->Ibytes != GATEWAY_RT_INPUT_SIZE_BYTES) ||
        (slave->outputs == NULL) || (slave->inputs == NULL)) {
        fprintf(stderr,
                "Unexpected PDO layout: output=%dB input=%dB (expected %uB/%uB)\n",
                slave->Obytes,
                slave->Ibytes,
                GATEWAY_RT_OUTPUT_SIZE_BYTES,
                GATEWAY_RT_INPUT_SIZE_BYTES);
        return false;
    }

    master->expected_wkc = (master->soem.grouplist[0].outputsWKC * 2) +
                           master->soem.grouplist[0].inputsWKC;
    if (master->expected_wkc <= 0) {
        fprintf(stderr, "Invalid expected WKC: %d\n", master->expected_wkc);
        return false;
    }

    printf("Gateway: slave=%d, PDO output=%dB input=%dB, expected WKC=%d\n",
           master->slave_index,
           slave->Obytes,
           slave->Ibytes,
           master->expected_wkc);
    return enter_operational(master);
}

/// @brief 在显式配置时请求 SCHED_FIFO 调度策略。
/// @param priority 请求的优先级；小于等于零时保持默认调度策略。
/// @note 申请失败仅打印警告，主站仍按普通调度策略运行。
static void set_realtime_priority(int priority)
{
    struct sched_param parameters = {0};

    if (priority <= 0) {
        return;
    }

    parameters.sched_priority = priority;
    if (sched_setscheduler(0, SCHED_FIFO, &parameters) != 0) {
        fprintf(stderr, "Warning: SCHED_FIFO priority %d was not applied: %s\n",
                priority,
                strerror(errno));
    }
}

/// @brief 输出每秒一次的周期、WKC 和时序统计。
/// @param stats 由周期路径维护的统计数据。
/// @param previous_cycles 上一次报告时的累计周期数。
/// @note 控制台输出位于周期路径外，以避免破坏实时性。
static void report_stats(const gateway_rt_runtime_stats *stats, uint64_t previous_cycles)
{
    uint64_t cycles = atomic_load_explicit(&stats->cycles, memory_order_relaxed);
    uint64_t wkc_errors = atomic_load_explicit(&stats->wkc_errors, memory_order_relaxed);
    uint64_t deadline_misses = atomic_load_explicit(&stats->deadline_misses, memory_order_relaxed);
    uint64_t max_exchange_us = atomic_load_explicit(&stats->max_exchange_us, memory_order_relaxed);
    uint64_t max_lateness_us = atomic_load_explicit(&stats->max_lateness_us, memory_order_relaxed);

    printf("cycles/s=%" PRIu64 " total=%" PRIu64 " wkc_errors=%" PRIu64
           " deadline_misses=%" PRIu64 " max_exchange_us=%" PRIu64
           " max_lateness_us=%" PRIu64 "\n",
           cycles - previous_cycles,
           cycles,
           wkc_errors,
           deadline_misses,
           max_exchange_us,
           max_lateness_us);
}

/// @brief 输出当前周期接收到的 IMU 有效字段及 16 轴反馈。
/// @param input 本周期从 HPM 网关接收的固定 252 字节输入 PDO。
/// @note 此函数仅在显式指定 --print-feedback 时每秒调用一次。
static void print_feedback(const gateway_rt_input_pdo *input)
{
    const gateway_rt_imu_feedback *imu = &input->imu;
    uint32_t missing_fields = GATEWAY_RT_IMU_REQUIRED_FIELDS & ~imu->valid_fields;
    uint32_t axis;

    printf("IMU: status=0x%08" PRIX32 " valid=0x%08" PRIX32
           " missing=0x%08" PRIX32 " tick=%" PRIu32 " sample_us=%" PRIu32 "\n",
           imu->status,
           imu->valid_fields,
           missing_fields,
           imu->received_tick,
           imu->sample_timestamp_us);
    printf("IMU: acc=%.3f,%.3f,%.3f m/s^2 gyro=%.3f,%.3f,%.3f dps "
           "quat=%.4f,%.4f,%.4f,%.4f temp=%.2f C\n",
           imu->acceleration_mps2[0],
           imu->acceleration_mps2[1],
           imu->acceleration_mps2[2],
           imu->angular_rate_dps[0],
           imu->angular_rate_dps[1],
           imu->angular_rate_dps[2],
           imu->quaternion[0],
           imu->quaternion[1],
           imu->quaternion[2],
           imu->quaternion[3],
           imu->temperature_c);

    for (axis = 0U; axis < GATEWAY_RT_AXIS_COUNT; axis++) {
        const gateway_rt_motor_feedback *feedback = &input->axis[axis];

        printf("axis%02" PRIu32 ": pos=%.4f rad vel=%.4f rad/s torque=%.4f Nm\n",
               axis,
               feedback->position_rad,
               feedback->velocity_rad_s,
               feedback->torque_nm);
    }
}

/// @brief 独占执行固定周期的 EtherCAT PDO 发送与接收事务。
/// @param master 已进入 OP 的主站实例；本函数是 SOEM 周期访问的唯一所有者。
/// @param options 已校验的周期和接收超时配置。
/// @note 循环持续至 SIGINT 或 SIGTERM 设置停止标志。
static void run_cyclic(gateway_rt_master *master, const gateway_rt_options *options)
{
    ec_slavet *slave = &master->soem.slavelist[master->slave_index];
    struct timespec next_release;
    uint64_t next_report_us;
    uint64_t previous_cycles = 0U;

    /* 使用单调绝对时间释放周期，避免前一周期的收发耗时累计成漂移。 */
    (void)clock_gettime(CLOCK_MONOTONIC, &next_release);
    next_release = add_us(next_release, options->cycle_us);
    next_report_us = monotonic_time_us() + UINT64_C(1000000);

    while (!s_stop_requested) {
        uint64_t exchange_start_us;
        uint64_t exchange_end_us;
        uint64_t next_release_us;
        int wkc;

        /* 保持 EtherCAT 时序：一帧命令后必须恰好接收对应的一帧响应。 */
        memcpy(slave->outputs, &master->output, sizeof(master->output));
        exchange_start_us = monotonic_time_us();
        (void)ecx_send_processdata(&master->soem);
        wkc = ecx_receive_processdata(&master->soem, (int)options->rx_timeout_us);
        exchange_end_us = monotonic_time_us();

        atomic_fetch_add_explicit(&master->stats.cycles, 1U, memory_order_relaxed);
        update_max_atomic(&master->stats.max_exchange_us, exchange_end_us - exchange_start_us);
        if (wkc != master->expected_wkc) {
            atomic_fetch_add_explicit(&master->stats.wkc_errors, 1U, memory_order_relaxed);
        }

        next_release_us = ((uint64_t)next_release.tv_sec * UINT64_C(1000000)) +
                          ((uint64_t)next_release.tv_nsec / 1000U);
        if (exchange_end_us > next_release_us) {
            /* 发生超期时跳过过期释放点，避免补发紧挨的 PDO 帧。 */
            atomic_fetch_add_explicit(&master->stats.deadline_misses, 1U, memory_order_relaxed);
            update_max_atomic(&master->stats.max_lateness_us, exchange_end_us - next_release_us);
            do {
                next_release = add_us(next_release, options->cycle_us);
                next_release_us += options->cycle_us;
            } while (exchange_end_us > next_release_us);
        }

        if (exchange_end_us >= next_report_us) {
            report_stats(&master->stats, previous_cycles);
            if (options->print_feedback) {
                gateway_rt_input_pdo input;

                /* 在已完成的交换后复制，确保本次报告使用同一份 PDO 映像。 */
                memcpy(&input, slave->inputs, sizeof(input));
                print_feedback(&input);
            }
            previous_cycles = atomic_load_explicit(&master->stats.cycles, memory_order_relaxed);
            next_report_us = exchange_end_us + UINT64_C(1000000);
        }

        while ((clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_release, NULL) == EINTR) &&
               !s_stop_requested) {
        }
        next_release = add_us(next_release, options->cycle_us);
    }
}

/// @brief 发送一帧零 PDO，随后请求从站进入 INIT。
/// @param master 已初始化或部分初始化的主站实例。
/// @note 此操作仅是通信层停止；物理急停和断能必须由独立硬件安全链路完成。
static void stop_gateway(gateway_rt_master *master)
{
    ec_slavet *slave;

    if ((master->slave_index < 0) || !master->soem_initialized) {
        return;
    }

    slave = &master->soem.slavelist[master->slave_index];
    if (slave->outputs != NULL) {
        /* 零 PDO 仅用于通信层停止，不能替代硬件急停。 */
        memset(slave->outputs, 0, GATEWAY_RT_OUTPUT_SIZE_BYTES);
        (void)ecx_send_processdata(&master->soem);
        (void)ecx_receive_processdata(&master->soem, EC_TIMEOUTRET);
    }
    slave->state = EC_STATE_INIT;
    ecx_writestate(&master->soem, (uint16_t)master->slave_index);
    (void)wait_for_state(master, EC_STATE_INIT, EC_TIMEOUTRET);
}

/// @brief 在 EtherCAT 状态切换完成后释放 SOEM 原始套接字。
/// @param master 包含 SOEM 上下文的主站实例。
static void close_master(gateway_rt_master *master)
{
    if (master->soem_initialized) {
        ecx_close(&master->soem);
        master->soem_initialized = false;
    }
}

/// @brief 程序入口：解析参数、运行一次网关会话并执行配套停止流程。
/// @param argc 命令行参数数量。
/// @param argv 命令行参数数组。
/// @return 成功完成会话时返回 EXIT_SUCCESS，否则返回 EXIT_FAILURE。
int main(int argc, char **argv)
{
    gateway_rt_master master = { .slave_index = -1 };
    gateway_rt_options options;

    if (!parse_options(argc, argv, &options)) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
        fprintf(stderr, "Warning: mlockall was not applied: %s\n", strerror(errno));
    }
    signal(SIGINT, request_stop);
    signal(SIGTERM, request_stop);

    if (!configure_master(&master, &options)) {
        stop_gateway(&master);
        close_master(&master);
        return EXIT_FAILURE;
    }

    set_realtime_priority(options.rt_priority);

    fprintf(stderr,
            "WARNING: current gateway firmware auto-enables every configured axis. "
            "Reference motion is sent only to CAN0 IDs 1, 2, 3, and 13.\n");
    /* PDO axis n corresponds to global CAN motor ID n + 1. */
    // master.output.axis[12] = (gateway_rt_motor_command) {
    //     .position_rad = 0.0f,
    //     .velocity_rad_s = 0.6f,
    //     .feedforward_torque_nm = 0.0f,
    //     .kp_nm_per_rad = 0.0f,
    //     .kd_nm_per_rad_s = 1.1f,
    // };
    // master.output.axis[0] = (gateway_rt_motor_command) {
    //     .position_rad = 0.0f,
    //     .velocity_rad_s = 1.0f,
    //     .feedforward_torque_nm = 0.0f,
    //     .kp_nm_per_rad = 0.0f,
    //     .kd_nm_per_rad_s = 2.0f,
    // };
    // master.output.axis[1] = (gateway_rt_motor_command) {
    //     .position_rad = 0.0f,
    //     .velocity_rad_s = 1.0f,
    //     .feedforward_torque_nm = 0.0f,
    //     .kp_nm_per_rad = 0.0f,
    //     .kd_nm_per_rad_s = 2.0f,
    // };
    // master.output.axis[2] = (gateway_rt_motor_command) {
    //     .position_rad = 0.0f,
    //     .velocity_rad_s = 1.0f,
    //     .feedforward_torque_nm = 0.0f,
    //     .kp_nm_per_rad = 0.0f,
    //     .kd_nm_per_rad_s = 2.0f,
    // };
    run_cyclic(&master, &options);
    stop_gateway(&master);
    close_master(&master);
    return EXIT_SUCCESS;
}
