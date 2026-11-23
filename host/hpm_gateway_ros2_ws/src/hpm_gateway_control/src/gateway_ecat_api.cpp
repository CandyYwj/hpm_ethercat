#define _POSIX_C_SOURCE 200809L

#include "hpm_gateway_control/gateway_ecat_api.hpp"

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <memory>
#include <sched.h>
#include <sys/mman.h>
#include <thread>
#include <time.h>
#include <utility>
#include <unistd.h>

#include "hpm_gateway_control/latest_buffer.hpp"
#include "soem/soem.h"

namespace hpm_gateway_control {
namespace {

constexpr std::size_t kIomapSizeBytes = 4096U;
constexpr std::uint32_t kMaxCycleUs = 100000U;
constexpr int kMaxRtPriority = 99;

std::uint64_t monotonic_time_us()
{
    struct timespec now {};
    (void)clock_gettime(CLOCK_MONOTONIC, &now);
    return static_cast<std::uint64_t>(now.tv_sec) * 1000000U +
           static_cast<std::uint64_t>(now.tv_nsec) / 1000U;
}

timespec add_us(timespec time, std::uint32_t microseconds)
{
    time.tv_nsec += static_cast<long>(microseconds % 1000000U) * 1000L;
    time.tv_sec += static_cast<time_t>(microseconds / 1000000U);
    if (time.tv_nsec >= 1000000000L) {
        ++time.tv_sec;
        time.tv_nsec -= 1000000000L;
    }
    return time;
}

void update_max(std::atomic<std::uint64_t> & target, std::uint64_t value)
{
    auto observed = target.load(std::memory_order_relaxed);
    while (value > observed &&
           !target.compare_exchange_weak(
               observed, value, std::memory_order_relaxed,
               std::memory_order_relaxed)) {
    }
}

}  // namespace

struct GatewayEcat::Impl final {
    explicit Impl(GatewayEcatOptions input_options)
        : options(std::move(input_options))
    {
    }

    GatewayEcatOptions options;
    std::thread worker;
    std::atomic<bool> stop_requested{false};
    std::atomic<bool> running{false};
    std::atomic<bool> startup_done{false};
    std::atomic<bool> startup_ok{false};
    std::atomic<std::uint64_t> cycles{0U};
    std::atomic<std::uint64_t> wkc_errors{0U};
    std::atomic<std::uint64_t> deadline_misses{0U};
    std::atomic<std::uint64_t> command_timeouts{0U};
    std::atomic<std::uint64_t> feedback_dropped{0U};
    std::atomic<std::uint64_t> max_exchange_us{0U};
    std::atomic<std::uint64_t> max_lateness_us{0U};
    LatestBuffer<gateway_rt_command_frame> command_buffer;
    LatestBuffer<gateway_rt_feedback_frame> feedback_buffer;

    ecx_contextt soem{};
    bool soem_initialized{false};
    int slave_index{-1};
    int expected_wkc{0};
    std::uint8_t iomap[kIomapSizeBytes]{};

    bool configure_master();
    bool enter_operational();
    bool wait_for_state(std::uint16_t expected_state, int timeout_us);
    void run();
    void run_cyclic();
    void stop_gateway();
    void close_master();
    void set_realtime_priority();
    void set_cpu_affinity();
    int find_gateway() const;
};

int GatewayEcat::Impl::find_gateway() const
{
    for (int slave = 1; slave <= soem.slavecount; ++slave) {
        const ec_slavet & candidate = soem.slavelist[slave];
        if (candidate.eep_man == GATEWAY_RT_VENDOR_ID &&
            candidate.eep_id == GATEWAY_RT_PRODUCT_CODE) {
            return slave;
        }
    }
    return -1;
}

bool GatewayEcat::Impl::wait_for_state(
    std::uint16_t expected_state, int timeout_us)
{
    const std::uint16_t actual = ecx_statecheck(
        &soem, static_cast<std::uint16_t>(slave_index),
        expected_state, timeout_us);
    return (actual & 0x000FU) == expected_state;
}

bool GatewayEcat::Impl::enter_operational()
{
    ec_slavet * slave = &soem.slavelist[slave_index];
    std::memset(slave->outputs, 0, GATEWAY_RT_OUTPUT_SIZE_BYTES);

    for (int attempt = 0; attempt < 500 && !stop_requested.load(); ++attempt) {
        const int wkc = ecx_send_processdata(&soem);
        (void)ecx_receive_processdata(&soem, EC_TIMEOUTRET);
        slave->state = EC_STATE_OPERATIONAL;
        ecx_writestate(&soem, static_cast<std::uint16_t>(slave_index));
        if (wait_for_state(EC_STATE_OPERATIONAL, EC_TIMEOUTRET)) {
            std::fprintf(
                stderr, "Gateway entered OP (initial WKC=%d, expected=%d)\n",
                wkc, expected_wkc);
            return true;
        }
        usleep(10000U);
    }

    std::fprintf(
        stderr, "Gateway did not enter OP: AL=0x%04X (%s)\n",
        slave->ALstatuscode, ec_ALstatuscode2string(slave->ALstatuscode));
    return false;
}

bool GatewayEcat::Impl::configure_master()
{
    if (options.interface_name.empty() ||
        options.cycle_us < 500U ||
        options.cycle_us > kMaxCycleUs ||
        options.rx_timeout_us == 0U ||
        options.rx_timeout_us >= options.cycle_us ||
        options.command_timeout_ms == 0U ||
        options.rt_priority < 0 ||
        options.rt_priority > kMaxRtPriority ||
        options.rt_cpu < -1) {
        std::fprintf(stderr, "Invalid EtherCAT options\n");
        return false;
    }

    if (!ecx_init(&soem, options.interface_name.c_str())) {
        std::fprintf(stderr, "ecx_init(%s) failed\n",
                     options.interface_name.c_str());
        return false;
    }
    soem_initialized = true;

    if (ecx_config_init(&soem) <= 0) {
        std::fprintf(stderr, "No EtherCAT slaves found\n");
        return false;
    }

    slave_index = find_gateway();
    if (slave_index < 0) {
        std::fprintf(stderr, "HPM gateway was not found\n");
        return false;
    }

    (void)ecx_config_map_group(&soem, iomap, 0U);
    ecx_configdc(&soem);
    ec_slavet * slave = &soem.slavelist[slave_index];

    if (slave->Obytes != static_cast<int>(GATEWAY_RT_OUTPUT_SIZE_BYTES) ||
        slave->Ibytes != static_cast<int>(GATEWAY_RT_INPUT_SIZE_BYTES) ||
        slave->outputs == nullptr || slave->inputs == nullptr) {
        std::fprintf(
            stderr, "Unexpected PDO layout: output=%dB input=%dB\n",
            slave->Obytes, slave->Ibytes);
        return false;
    }

    expected_wkc = (soem.grouplist[0].outputsWKC * 2) +
                   soem.grouplist[0].inputsWKC;
    if (expected_wkc <= 0) {
        std::fprintf(stderr, "Invalid expected WKC: %d\n", expected_wkc);
        return false;
    }

    std::fprintf(
        stderr, "Gateway: slave=%d, PDO output=%dB input=%dB expected WKC=%d\n",
        slave_index, slave->Obytes, slave->Ibytes, expected_wkc);
    return enter_operational();
}

void GatewayEcat::Impl::set_realtime_priority()
{
    if (options.rt_priority == 0) {
        return;
    }

    struct sched_param parameters {};
    parameters.sched_priority = options.rt_priority;
    if (sched_setscheduler(0, SCHED_FIFO, &parameters) != 0) {
        std::fprintf(
            stderr, "Warning: SCHED_FIFO priority %d was not applied: %s\n",
            options.rt_priority, std::strerror(errno));
    }
}

/* Pin the ECAT worker thread to a dedicated core so SCHED_FIFO does not
 * starve the ROS executor threads and the thread keeps its L1/L2 cache. */
void GatewayEcat::Impl::set_cpu_affinity()
{
    if (options.rt_cpu < 0) {
        return;
    }

    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(options.rt_cpu, &set);
    if (sched_setaffinity(0, sizeof(set), &set) != 0) {
        std::fprintf(
            stderr, "Warning: CPU affinity core %d was not applied: %s\n",
            options.rt_cpu, std::strerror(errno));
    }
}

void GatewayEcat::Impl::run_cyclic()
{
    ec_slavet * slave = &soem.slavelist[slave_index];
    gateway_rt_output_pdo output{};
    gateway_rt_command_frame command{};
    std::uint64_t last_command_us = 0U;
    std::uint32_t sequence = 0U;
    std::uint64_t feedback_sequence = 0U;
    bool command_timed_out = false;
    timespec next_release {};
    (void)clock_gettime(CLOCK_MONOTONIC, &next_release);
    next_release = add_us(next_release, options.cycle_us);

    while (!stop_requested.load(std::memory_order_relaxed)) {
        const std::uint64_t exchange_start_us = monotonic_time_us();

        if (command_buffer.read(command)) {
            output = command.output;
            last_command_us = command.received_monotonic_us;
            sequence = command.sequence;
        }

        const std::uint64_t timeout_us =
            static_cast<std::uint64_t>(options.command_timeout_ms) * 1000U;
        if (last_command_us == 0U ||
            exchange_start_us - last_command_us > timeout_us) {
            std::memset(&output, 0, sizeof(output));
            if (!command_timed_out) {
                command_timeouts.fetch_add(1U, std::memory_order_relaxed);
            }
            command_timed_out = true;
        } else {
            command_timed_out = false;
        }

        /* Debug: fill the 12-byte diagnostic pad (bytes 320..331) with a
         * distinct 0xA0+i pattern. The gateway firmware dumps the raw tail so
         * we can see exactly where the ESC process-data delivery stops. */
        for (std::uint32_t pad_i = 0U; pad_i < GATEWAY_RT_PAD_BYTES; ++pad_i) {
            output.pad[pad_i] = static_cast<std::uint8_t>(0xA0U + pad_i);
        }

        std::memcpy(slave->outputs, &output, sizeof(output));

        /* Debug: dump the outgoing PDO once per second (1 kHz cycle) to confirm
         * what the master actually sends to the gateway. Prints only non-zero
         * axes: p, v, t, kp, kd per motor (global CAN motor ID n + 1). */
        if ((cycles.load(std::memory_order_relaxed) % 1000U) == 0U) {
            std::fprintf(stderr, "[PDO-TX] seq=%u\n", sequence);
            for (std::uint32_t debug_axis = 0U;
                 debug_axis < GATEWAY_RT_AXIS_COUNT; ++debug_axis) {
                const auto & debug_cmd = output.axis[debug_axis];
                if (debug_cmd.position_rad != 0.0f ||
                    debug_cmd.velocity_rad_s != 0.0f ||
                    debug_cmd.torque_nm != 0.0f ||
                    debug_cmd.kp_nm_per_rad != 0.0f ||
                    debug_cmd.kd_nm_per_rad_s != 0.0f) {
                    std::fprintf(
                        stderr,
                        "  motor %2u: p=%8.4f v=%8.4f t=%8.4f kp=%8.4f kd=%8.4f\n",
                        debug_axis + 1U, debug_cmd.position_rad,
                        debug_cmd.velocity_rad_s, debug_cmd.torque_nm,
                        debug_cmd.kp_nm_per_rad, debug_cmd.kd_nm_per_rad_s);
                }
            }
            /* Raw bytes SOEM will actually put on the wire: group output length,
             * the last 4 bytes of the send buffer (axis 15 kd), and the 12-byte
             * diagnostic pad (bytes 320..331). */
            float raw_kd15 = 0.0f;
            std::memcpy(&raw_kd15, slave->outputs + 316U, sizeof(raw_kd15));
            std::fprintf(stderr, "  sendbuf: groupObytes=%u ax15kd=%.4f pad=",
                         static_cast<unsigned>(soem.grouplist[0].Obytes),
                         static_cast<double>(raw_kd15));
            for (std::uint32_t pad_i = 0U; pad_i < GATEWAY_RT_PAD_BYTES; ++pad_i) {
                std::fprintf(stderr, "%02X",
                             static_cast<unsigned>(
                                 static_cast<std::uint8_t>(slave->outputs[320U + pad_i])));
            }
            std::fprintf(stderr, "\n");
            std::fflush(stderr);
        }

        (void)ecx_send_processdata(&soem);
        const int wkc = ecx_receive_processdata(
            &soem, static_cast<int>(options.rx_timeout_us));
        const std::uint64_t exchange_end_us = monotonic_time_us();

        cycles.fetch_add(1U, std::memory_order_relaxed);
        update_max(max_exchange_us, exchange_end_us - exchange_start_us);
        if (wkc != expected_wkc) {
            wkc_errors.fetch_add(1U, std::memory_order_relaxed);
        }

        const std::uint64_t next_release_us =
            static_cast<std::uint64_t>(next_release.tv_sec) * 1000000U +
            static_cast<std::uint64_t>(next_release.tv_nsec) / 1000U;
        if (exchange_end_us > next_release_us) {
            deadline_misses.fetch_add(1U, std::memory_order_relaxed);
            update_max(max_lateness_us, exchange_end_us - next_release_us);
            do {
                next_release = add_us(next_release, options.cycle_us);
            } while (
                exchange_end_us >
                static_cast<std::uint64_t>(next_release.tv_sec) * 1000000U +
                    static_cast<std::uint64_t>(next_release.tv_nsec) / 1000U);
        }

        ++feedback_sequence;
        gateway_rt_feedback_frame feedback{};
        feedback.received_monotonic_us = exchange_end_us;
        feedback.sequence = sequence;
        feedback.feedback_sequence = static_cast<std::uint32_t>(feedback_sequence);
        std::memcpy(&feedback.input, slave->inputs, sizeof(feedback.input));
        if (!feedback_buffer.publish(feedback)) {
            feedback_dropped.fetch_add(1U, std::memory_order_relaxed);
        }

        while (clock_nanosleep(
                   CLOCK_MONOTONIC, TIMER_ABSTIME, &next_release, nullptr) == EINTR &&
               !stop_requested.load(std::memory_order_relaxed)) {
        }
        next_release = add_us(next_release, options.cycle_us);
    }
}

void GatewayEcat::Impl::stop_gateway()
{
    if (!soem_initialized || slave_index < 0) {
        return;
    }

    ec_slavet * slave = &soem.slavelist[slave_index];
    if (slave->outputs != nullptr) {
        std::memset(slave->outputs, 0, GATEWAY_RT_OUTPUT_SIZE_BYTES);
        (void)ecx_send_processdata(&soem);
        (void)ecx_receive_processdata(&soem, EC_TIMEOUTRET);
    }
    slave->state = EC_STATE_INIT;
    ecx_writestate(&soem, static_cast<std::uint16_t>(slave_index));
    (void)wait_for_state(EC_STATE_INIT, EC_TIMEOUTRET);
}

void GatewayEcat::Impl::close_master()
{
    if (soem_initialized) {
        ecx_close(&soem);
        soem_initialized = false;
    }
}

void GatewayEcat::Impl::run()
{
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
        std::fprintf(stderr, "Warning: mlockall was not applied: %s\n",
                     std::strerror(errno));
    }

    const bool configured = configure_master();
    startup_ok.store(configured, std::memory_order_release);
    startup_done.store(true, std::memory_order_release);
    running.store(configured, std::memory_order_release);

    if (configured) {
        set_cpu_affinity();
        set_realtime_priority();
        run_cyclic();
        stop_gateway();
    }

    close_master();
    running.store(false, std::memory_order_release);
}

GatewayEcat::GatewayEcat(GatewayEcatOptions options)
    : impl_(std::make_unique<Impl>(std::move(options)))
{
}

GatewayEcat::~GatewayEcat()
{
    stop();
}

bool GatewayEcat::start()
{
    if (!impl_->options.arm_all_axes) {
        std::fprintf(stderr, "EtherCAT start requires arm_all_axes=true\n");
        return false;
    }
    if (impl_->worker.joinable()) {
        return impl_->running.load(std::memory_order_acquire);
    }

    impl_->stop_requested.store(false, std::memory_order_release);
    impl_->startup_done.store(false, std::memory_order_release);
    impl_->startup_ok.store(false, std::memory_order_release);
    impl_->worker = std::thread([this]() { impl_->run(); });

    while (!impl_->startup_done.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    if (!impl_->startup_ok.load(std::memory_order_acquire)) {
        impl_->worker.join();
        return false;
    }
    return true;
}

void GatewayEcat::stop() noexcept
{
    if (!impl_ || !impl_->worker.joinable()) {
        return;
    }

    impl_->stop_requested.store(true, std::memory_order_release);
    impl_->worker.join();
}

bool GatewayEcat::is_running() const noexcept
{
    return impl_->running.load(std::memory_order_acquire);
}

bool GatewayEcat::set_command(
    const gateway_rt_command_frame & command) noexcept
{
    return impl_->command_buffer.publish(command);
}

bool GatewayEcat::get_feedback(
    gateway_rt_feedback_frame & feedback) noexcept
{
    return impl_->feedback_buffer.read(feedback);
}

GatewayEcatStats GatewayEcat::stats() const noexcept
{
    return GatewayEcatStats{
        impl_->cycles.load(std::memory_order_relaxed),
        impl_->wkc_errors.load(std::memory_order_relaxed),
        impl_->deadline_misses.load(std::memory_order_relaxed),
        impl_->command_timeouts.load(std::memory_order_relaxed),
        impl_->feedback_dropped.load(std::memory_order_relaxed),
        impl_->max_exchange_us.load(std::memory_order_relaxed),
        impl_->max_lateness_us.load(std::memory_order_relaxed)};
}

}  // namespace hpm_gateway_control
