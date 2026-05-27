/**
 * @file    attitude.h
 * @brief   Cascaded PID attitude controller for X-frame quadcopter
 *
 * Architecture — two control loops per axis:
 *
 *  RC stick ──► [Outer: Angle P] ──► rate setpoint
 *                       ▲                   │
 *                  AHRS angles         [Inner: Rate PID] ──► motor offset (µs)
 *                                           ▲
 *                                      IMU gyro (deg/s)
 *
 *  Yaw uses a single rate-only loop (no absolute yaw reference).
 *
 *  Full update chain (called at 1 kHz):
 *    1. Read SBUS → derive angle / rate setpoints from sticks
 *    2. Outer Angle PID(roll, pitch) → roll_rate_sp, pitch_rate_sp
 *    3. Inner Rate  PID(roll, pitch, yaw) → roll_out, pitch_out, yaw_out
 *    4. Mixer_Compute(throttle, roll_out, pitch_out, yaw_out) → M1–M4
 *    5. Motor_SetAll(M1–M4)
 *
 *  Safety:
 *    - SBUS failsafe / timeout  → Motor_StopAll()
 *    - Throttle < THROTTLE_IDLE_US → I-terms frozen (no ground windup)
 *    - Disarm                   → Attitude_Reset() clears all PID state
 */

#ifndef FC_ATTITUDE_H
#define FC_ATTITUDE_H

#include <stdint.h>
#include "control/pid.h"

/* ── Controller state ──────────────────────────────────────────────── */

typedef struct {
    /* Outer loop — angle P-controllers (rate setpoint generators) */
    pid_ctrl_t roll_angle;
    pid_ctrl_t pitch_angle;

    /* Inner loop — full PID rate controllers */
    pid_ctrl_t roll_rate;
    pid_ctrl_t pitch_rate;
    pid_ctrl_t yaw_rate;
} attitude_ctrl_t;

/* ── Public API ────────────────────────────────────────────────────── */

/**
 * Initialise all 5 PID controllers with default gains from config.h.
 * Call once after AHRS_Init() and Motor_Arm().
 */
void Attitude_Init(attitude_ctrl_t *ctrl);

/**
 * Run one complete cascaded PID cycle.
 * Must be called at exactly MAIN_LOOP_RATE_HZ (1000 Hz) from the main loop.
 * Reads SBUS, AHRS and IMU; drives motors directly via Motor_SetAll().
 *
 * Returns 0 if motors were stopped due to failsafe, 1 otherwise.
 */
uint8_t Attitude_Update(attitude_ctrl_t *ctrl);

/**
 * Reset all five PID integrators and derivative states.
 * Call on disarm to prevent stale I-term on next arm.
 */
void Attitude_Reset(attitude_ctrl_t *ctrl);

#endif /* FC_ATTITUDE_H */
