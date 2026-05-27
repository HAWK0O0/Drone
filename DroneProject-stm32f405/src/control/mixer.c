/**
 * @file    mixer.c
 * @brief   X-frame quadcopter motor mixer
 */

#include "control/mixer.h"
#include "drivers/motor.h"
#include <stdint.h>

static inline uint16_t clamp_us(float v)
{
    if (v < MOTOR_PWM_MIN_US) return MOTOR_PWM_MIN_US;
    if (v > MOTOR_PWM_MAX_US) return MOTOR_PWM_MAX_US;
    return (uint16_t)v;
}

void Mixer_Compute(uint16_t throttle,
                   float roll, float pitch, float yaw,
                   mixer_output_t *out)
{
    float thr = (float)throttle;

    /*
     * X-frame layout (top view, CW = clockwise):
     *   M1 FL (CW)  ─┐  ┌─  M2 FR (CCW)
     *                  ><
     *   M3 RL (CCW) ─┘  └─  M4 RR (CW)
     */
    out->m1 = clamp_us(thr + roll - pitch + yaw);  /* Front-Left  */
    out->m2 = clamp_us(thr - roll - pitch - yaw);  /* Front-Right */
    out->m3 = clamp_us(thr + roll + pitch - yaw);  /* Rear-Left   */
    out->m4 = clamp_us(thr - roll + pitch + yaw);  /* Rear-Right  */
}
