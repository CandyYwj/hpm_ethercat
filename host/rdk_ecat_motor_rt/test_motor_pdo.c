/*
 * Author: BearKing
 */
#include "motor_pdo.h"

#include <stddef.h>
#include <stdio.h>

int main(void)
{
    if (offsetof(gateway_rt_input_pdo, axis) != GATEWAY_RT_IMU_INPUT_SIZE_BYTES) {
        fprintf(stderr, "motor feedback must follow the IMU prefix\n");
        return 1;
    }
    if (offsetof(gateway_rt_output_pdo, axis[1]) != sizeof(gateway_rt_motor_command)) {
        fprintf(stderr, "motor command axes must be contiguous\n");
        return 1;
    }

    puts("motor_pdo_test: PASS");
    return 0;
}
