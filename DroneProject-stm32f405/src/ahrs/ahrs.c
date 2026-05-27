/**
 * @file    ahrs.c
 * @brief   AHRS wrapper — feeds IMU data through Madgwick and exposes state
 */

#include "ahrs/ahrs.h"
#include "ahrs/madgwick.h"
#include "drivers/imu.h"
#include "config.h"
#include <math.h>

#define RAD_TO_DEG  (180.0f / 3.14159265f)

static madgwick_t  s_filter;
static ahrs_state_t s_state = {0};
static uint32_t     s_update_count = 0;

void AHRS_Init(void)
{
    Madgwick_Init(&s_filter,
                  (float)ATTITUDE_RATE_HZ,
                  MADGWICK_BETA);
    s_update_count = 0;
}

void AHRS_Update(void)
{
    imu_data_t imu;
    IMU_Read(&imu);
    IMU_ApplyCalibration(&imu);

    Madgwick_Update6DOF(&s_filter,
                        imu.gyro_x,  imu.gyro_y,  imu.gyro_z,
                        imu.accel_x, imu.accel_y, imu.accel_z);

    float roll_r, pitch_r, yaw_r;
    Madgwick_GetEuler(&s_filter, &roll_r, &pitch_r, &yaw_r);

    s_state.roll  = roll_r  * RAD_TO_DEG;
    s_state.pitch = pitch_r * RAD_TO_DEG;
    s_state.yaw   = yaw_r   * RAD_TO_DEG;

    /* Map yaw from [-180, +180] → [0, 360] for display / heading use */
    if (s_state.yaw < 0.0f) {
        s_state.yaw += 360.0f;
    }

    s_state.q0 = s_filter.q0; s_state.q1 = s_filter.q1;
    s_state.q2 = s_filter.q2; s_state.q3 = s_filter.q3;

    s_update_count++;
}

const ahrs_state_t *AHRS_GetState(void)
{
    return &s_state;
}

uint8_t AHRS_IsConverged(void)
{
    /* Consider converged after 2 seconds of updates at ATTITUDE_RATE_HZ */
    return (s_update_count > (ATTITUDE_RATE_HZ * 2U)) ? 1U : 0U;
}

void AHRS_Reset(void)
{
    Madgwick_Init(&s_filter, (float)ATTITUDE_RATE_HZ, MADGWICK_BETA);
    s_update_count = 0;
}
