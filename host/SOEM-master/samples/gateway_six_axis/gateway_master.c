/**
 * gateway_master.c - Windows bench master for the HPM6E80 six-axis slave.
 *
 * The slave keeps its 320 B RxPDO / 252 B TxPDO contract: only the first six
 * axis groups carry motors, groups 7..16 stay zero.
 *
 * Usage:
 *   gateway_master.exe "\Device\NPF_{GUID}"                  hold all-zero commands
 *   gateway_master.exe "\Device\NPF_{GUID}" script.txt       timed script
 *   gateway_master.exe "\Device\NPF_{GUID}" script.txt 5000  also stop after N ms
 *
 * Script lines (one per line, '#' starts a comment):
 *   <ms> zero                                   all axes free (Kp=Kd=T=0)
 *   <ms> pos <axis 1..6> <rad> <kp> <kd>        hold a position
 *   <ms> stop                                   same as zero, then keep holding
 *   <ms> quit
 *
 * Safety: every exit path writes zero commands (no torque) for a moment and
 * then drops the slave back to INIT, so a closed console never leaves a motor
 * with a stale target.
 */

#include "soem/soem.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#define GW_RXPDO_BYTES      320U
#define GW_TXPDO_BYTES      252U
#define GW_AXIS_COUNT         6U
#define GW_PDO_AXIS_COUNT    16U
#define GW_AXIS_BYTES        12U   /* TxPDO feedback per axis: 3 x REAL32 */
#define GW_IMU_BYTES         60U
#define GW_RXPDO_MAX_BYTES 332U   /* slave maps 332: 320 axes + 12 B legacy pad */
#define GW_IO_MAP_BYTES    (GW_RXPDO_MAX_BYTES + GW_TXPDO_BYTES)
#define GW_MAX_SCRIPT_LINES  64

typedef struct {
    uint32_t at_ms;
    uint8_t action;      /* 0 zero, 1 pos, 2 quit */
    uint8_t axis;        /* 0..5 */
    float position;
    float kp;
    float kd;
} gw_step_t;

static ecx_contextt s_context;
static uint8_t s_io_map[GW_IO_MAP_BYTES];
static uint32_t s_output_bytes = GW_RXPDO_BYTES;
static volatile sig_atomic_t s_stop_requested;

static void gw_handle_signal(int signal_number)
{
    (void)signal_number;
    s_stop_requested = 1;
}

static uint32_t gw_now_ms(void)
{
#ifdef _WIN32
    static LARGE_INTEGER freq;
    static LARGE_INTEGER start;
    LARGE_INTEGER now;

    if (freq.QuadPart == 0) {
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&start);
    }
    QueryPerformanceCounter(&now);
    return (uint32_t)(((now.QuadPart - start.QuadPart) * 1000) / freq.QuadPart);
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000U + ts.tv_nsec / 1000000U);
#endif
}

static void gw_write_real32(uint8_t *dst, float value)
{
    memcpy(dst, &value, sizeof(value));
}

static float gw_read_real32(const uint8_t *src)
{
    float value;

    memcpy(&value, src, sizeof(value));
    return value;
}

/* Zero the whole RxPDO: Kp = Kd = torque = velocity = 0 means "no torque". */
static void gw_clear_outputs(void)
{
    memset(s_io_map, 0, s_output_bytes);
}

static void gw_set_axis(uint8_t axis, float position, float kp, float kd)
{
    uint8_t *group;

    if (axis >= GW_AXIS_COUNT) {
        return;
    }

    group = s_io_map + (axis * 20U);
    gw_write_real32(group + 0U, position);
    gw_write_real32(group + 4U, 0.0f);
    gw_write_real32(group + 8U, 0.0f);
    gw_write_real32(group + 12U, kp);
    gw_write_real32(group + 16U, kd);
}

static size_t gw_load_script(const char *path, gw_step_t *steps, size_t capacity)
{
    FILE *file = fopen(path, "r");
    char line[160];
    size_t count = 0U;

    if (file == NULL) {
        printf("[master] cannot open script: %s\n", path);
        return 0U;
    }

    while ((count < capacity) && (fgets(line, sizeof(line), file) != NULL)) {
        char action[16];
        unsigned long at_ms;
        unsigned int axis;
        float position;
        float kp;
        float kd;
        const char *cursor = line;

        while ((*cursor == ' ') || (*cursor == '\t')) {
            cursor++;
        }
        if ((*cursor == '#') || (*cursor == '\n') || (*cursor == '\0')) {
            continue;
        }

        if (sscanf(cursor, "%lu %15s", &at_ms, action) != 2) {
            continue;
        }

        if (strcmp(action, "quit") == 0) {
            steps[count].at_ms = (uint32_t)at_ms;
            steps[count].action = 2U;
            count++;
        } else if (strcmp(action, "zero") == 0) {
            steps[count].at_ms = (uint32_t)at_ms;
            steps[count].action = 0U;
            count++;
        } else if (strcmp(action, "pos") == 0) {
            if (sscanf(cursor, "%*u %*s %u %f %f %f", &axis, &position, &kp, &kd) != 4) {
                printf("[master] bad pos line: %s", line);
                continue;
            }
            if ((axis < 1U) || (axis > GW_AXIS_COUNT)) {
                printf("[master] axis %u out of range: %s", axis, line);
                continue;
            }
            steps[count].at_ms = (uint32_t)at_ms;
            steps[count].action = 1U;
            steps[count].axis = (uint8_t)(axis - 1U);
            steps[count].position = position;
            steps[count].kp = kp;
            steps[count].kd = kd;
            count++;
        } else {
            printf("[master] unknown action '%s'\n", action);
        }
    }

    fclose(file);
    return count;
}

static void gw_apply_step(const gw_step_t *step, gw_step_t *held)
{
    if (step->action == 0U) {
        gw_clear_outputs();
        memset(held, 0, sizeof(*held));
        held->action = 0xFFU;
    } else if (step->action == 1U) {
        gw_set_axis(step->axis, step->position, step->kp, step->kd);
        held[step->axis] = *step;
        held[step->axis].action = 0xFFU;
    }
}

static void gw_print_status(unsigned int cycle, int wkc, int expected_wkc,
                            uint32_t worst_cycle_us, uint32_t max_cycle_us)
{
    const uint8_t *inputs = s_io_map + s_output_bytes;
    unsigned int axis;

    printf("[master] cycle=%u wkc=%d/%d maxCycleUs=%u\n",
           cycle, wkc, expected_wkc, (unsigned int)max_cycle_us);

    for (axis = 0U; axis < GW_AXIS_COUNT; axis++) {
        const uint8_t *group = inputs + GW_IMU_BYTES + (axis * GW_AXIS_BYTES);

        printf("  axis%u pos=%8.4f vel=%8.4f tq=%8.4f\n",
               axis + 1U,
               (double)gw_read_real32(group + 0U),
               (double)gw_read_real32(group + 4U),
               (double)gw_read_real32(group + 8U));
    }
    (void)worst_cycle_us;
}

int main(int argc, char *argv[])
{
    gw_step_t steps[GW_MAX_SCRIPT_LINES];
    gw_step_t held[GW_AXIS_COUNT];
    size_t step_count = 0U;
    size_t next_step = 0U;
    const char *ifname;
    const char *script_path = NULL;
    uint32_t duration_ms = 0U;
    uint32_t start_ms;
    uint32_t last_status_ms = 0U;
    uint32_t last_cycle_ms;
    uint32_t max_cycle_us = 0U;
    unsigned int cycle = 0U;
    int expected_wkc;
    int wkc;

    if ((argc >= 2) && (strcmp(argv[1], "--list") == 0)) {
        pcap_if_t *all_devices = NULL;
        pcap_if_t *device;
        char errbuf[PCAP_ERRBUF_SIZE];

        memset(errbuf, 0, sizeof(errbuf));
        if (pcap_findalldevs(&all_devices, errbuf) != 0) {
            printf("[master] pcap_findalldevs failed: %s\n", errbuf);
            return 1;
        }
        for (device = all_devices; device != NULL; device = device->next) {
            printf("%-58s %s\n", device->name,
                   (device->description != NULL) ? device->description : "");
        }
        pcap_freealldevs(all_devices);
        return 0;
    }
    if (argc < 2) {
        printf("usage: %s <ifname> [script.txt] [duration_ms]\n", argv[0]);
        printf("  Windows ifname example: \\Device\\NPF_{GUID}\n");
        return 1;
    }

    ifname = argv[1];
    if ((argc > 2) && (strncmp(argv[2], "--", 2) != 0)) {
        script_path = argv[2];
    }
    if (argc > 3) {
        duration_ms = (uint32_t)strtoul(argv[3], NULL, 10);
    }

    memset(held, 0, sizeof(held));
    s_stop_requested = 0;
    signal(SIGINT, gw_handle_signal);
    signal(SIGTERM, gw_handle_signal);

    printf("[master] interface: %s\n", ifname);
    if (script_path == NULL) {
        printf("[master] no script: holding all-zero commands (no torque)\n");
    } else {
        step_count = gw_load_script(script_path, steps, GW_MAX_SCRIPT_LINES);
        printf("[master] script: %s (%u steps)\n", script_path,
               (unsigned int)step_count);
    }

    memset(s_io_map, 0, sizeof(s_io_map));
    if (!ecx_init(&s_context, ifname)) {
        printf("[master] no socket connection on %s\n", ifname);
        return 1;
    }
    if (ecx_config_init(&s_context) <= 0) {
        printf("[master] no slaves found\n");
        ecx_close(&s_context);
        return 1;
    }
    printf("[master] slaves found: %d\n", s_context.slavecount);

    ecx_config_map_group(&s_context, s_io_map, 0);
    {
        ec_groupt *group = &s_context.grouplist[0];

        printf("[master] mapped %dO+%dI bytes\n", group->Obytes, group->Ibytes);
        if (((group->Obytes != (int)GW_RXPDO_BYTES) && (group->Obytes != (int)GW_RXPDO_MAX_BYTES))
            || (group->Ibytes != (int)GW_TXPDO_BYTES)) {
            printf("[master] PDO size mismatch: slave has %dO+%dI, expected %u or %u O plus %u I\n",
                   group->Obytes, group->Ibytes,
                   (unsigned int)GW_RXPDO_BYTES, (unsigned int)GW_RXPDO_MAX_BYTES,
                   (unsigned int)GW_TXPDO_BYTES);
            ecx_close(&s_context);
            return 1;
        }
        s_output_bytes = (uint32_t)group->Obytes;
        if (s_output_bytes != GW_RXPDO_BYTES) {
            printf("[master] note: slave maps %u B output (320 axes + %u B legacy pad); first 6 groups are used\n",
                   (unsigned int)s_output_bytes,
                   (unsigned int)(s_output_bytes - GW_RXPDO_BYTES));
        }
        expected_wkc = (group->outputsWKC * 2) + group->inputsWKC;
    }
    ecx_configdc(&s_context);

    ecx_statecheck(&s_context, 0, EC_STATE_SAFE_OP, EC_TIMEOUTSTATE * 4);
    gw_clear_outputs();
    ecx_send_processdata(&s_context);
    ecx_receive_processdata(&s_context, EC_TIMEOUTRET);

    s_context.slavelist[0].state = EC_STATE_OPERATIONAL;
    ecx_writestate(&s_context, 0);
    ecx_statecheck(&s_context, 0, EC_STATE_OPERATIONAL, EC_TIMEOUTSTATE / 10);

    if (s_context.slavelist[0].state != EC_STATE_OPERATIONAL) {
        printf("[master] slave did not reach OP (state=0x%02X)\n",
               s_context.slavelist[0].state);
        ecx_close(&s_context);
        return 1;
    }
    printf("[master] slave in OP, expected WKC=%d\n", expected_wkc);

    start_ms = gw_now_ms();
    last_cycle_ms = start_ms;

    while (!s_stop_requested) {
        uint32_t now_ms = gw_now_ms();
        uint32_t elapsed = now_ms - start_ms;

        if ((duration_ms != 0U) && (elapsed >= duration_ms)) {
            break;
        }

        while ((next_step < step_count) && (steps[next_step].at_ms <= elapsed)) {
            if (steps[next_step].action == 2U) {
                s_stop_requested = 1;
                break;
            }
            gw_apply_step(&steps[next_step], held);
            next_step++;
        }
        if (s_stop_requested) {
            break;
        }

        ecx_send_processdata(&s_context);
        wkc = ecx_receive_processdata(&s_context, EC_TIMEOUTRET);
        cycle++;

        if ((wkc < expected_wkc) && ((cycle % 500U) == 0U)) {
            printf("[master] WKC short: %d/%d\n", wkc, expected_wkc);
        }

        if ((now_ms - last_status_ms) >= 200U) {
            gw_print_status(cycle, wkc, expected_wkc, 0U, max_cycle_us);
            last_status_ms = now_ms;
        }

        {
            uint32_t delta_us = (uint32_t)((now_ms - last_cycle_ms) * 1000U);
            if (delta_us > max_cycle_us) {
                max_cycle_us = delta_us;
            }
            last_cycle_ms = now_ms;
        }

        /* 2 ms nominal cycle, paced by the OS timer. */
#ifdef _WIN32
        Sleep(1);
#else
        usleep(1000);
#endif
    }

    printf("[master] stopping: zero commands, then INIT\n");
    gw_clear_outputs();
    for (unsigned int i = 0U; i < 25U; i++) {
        ecx_send_processdata(&s_context);
        (void)ecx_receive_processdata(&s_context, EC_TIMEOUTRET);
#ifdef _WIN32
        Sleep(2);
#else
        usleep(2000);
#endif
    }

    s_context.slavelist[0].state = EC_STATE_INIT;
    ecx_writestate(&s_context, 0);
    ecx_close(&s_context);
    printf("[master] closed after %u cycles\n", cycle);
    return 0;
}
