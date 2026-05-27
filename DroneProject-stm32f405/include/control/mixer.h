/**
 * @file    mixer.h
 * @brief   X-frame quadcopter motor mixer
 *
 *          Layout (top view):
 *             M1(CW)  M2(CCW)
 *              [FL]   [FR]
 *                  \ /
 *                  / \
 *              [RL]   [RR]
 *             M3(CCW) M4(CW)
 *
 *  Mix equations:
 *    M1 = throttle + roll - pitch + yaw
 *    M2 = throttle - roll - pitch - yaw
 *    M3 = throttle + roll + pitch - yaw
 *    M4 = throttle - roll + pitch + yaw
 */

#ifndef FC_MIXER_H
#define FC_MIXER_H

#include <stdint.h>

typedef struct {
    uint16_t m1;   /* PWM µs [1000–2000] */
    uint16_t m2;
    uint16_t m3;
    uint16_t m4;
} mixer_output_t;

/**
 * Compute motor outputs from flight controller demands.
 * @param throttle  [1000–2000] µs — collective thrust
 * @param roll      PID output — positive = roll right
 * @param pitch     PID output — positive = pitch forward
 * @param yaw       PID output — positive = yaw right (CW)
 * @param out       pointer to output struct to fill
 */
void Mixer_Compute(uint16_t throttle,
                   float roll, float pitch, float yaw,
                   mixer_output_t *out);

#endif /* FC_MIXER_H */
