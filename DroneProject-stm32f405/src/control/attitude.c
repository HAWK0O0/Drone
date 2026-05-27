/**
 * @file    attitude.c
 * @brief   Cascaded PID attitude controller — X-frame quadcopter
 *
 * Loop structure (both running at 1 kHz):
 *
 *   ┌── OUTER LOOP (Angle) ─────────────────────────────────────────────┐
 *   │  Input  : RC stick → desired angle (deg)                          │
 *   │           AHRS state → actual angle (deg)                         │
 *   │  PID    : P-only (ki=0, kd=0)  — standard for outer angle loop    │
 *   │  Output : desired rotation rate (deg/s) → setpoint for inner loop │
 *   └───────────────────────────────────────────────────────────────────┘
 *
 *   ┌── INNER LOOP (Rate) ──────────────────────────────────────────────┐
 *   │  Input  : rate setpoint (deg/s) from outer loop                   │
 *   │           IMU gyroscope → actual rate (deg/s)                     │
 *   │  PID    : Full PID with anti-windup + derivative low-pass filter  │
 *   │  Output : motor offset value (µs), fed into Mixer_Compute()       │
 *   └───────────────────────────────────────────────────────────────────┘
 *
 *   Yaw: single rate loop — RC stick → yaw rate setpoint (deg/s)
 *
 * RC stick mapping (SBUS channels, mapped to µs [1000–2000]):
 *   CH0 (AILERON)  → roll  setpoint: 1500 µs = 0°, full deflection = ±MAX_ROLL_ANGLE_DEG
 *   CH1 (ELEVATOR) → pitch setpoint: 1500 µs = 0°, full = ±MAX_PITCH_ANGLE_DEG
 *   CH2 (THROTTLE) → throttle µs: passed directly to mixer [1000–2000]
 *   CH3 (RUDDER)   → yaw rate:    1500 µs = 0 deg/s, full = ±MAX_YAW_RATE_DPS
 *   CH4 (ARM)      → arm switch (handled by safety module, not here)
 */

#include "control/attitude.h"
#include "control/mixer.h"
#include "drivers/motor.h"
#include "drivers/sbus.h"
#include "drivers/imu.h"
#include "ahrs/ahrs.h"
#include "config.h"

/* ── Local helpers ─────────────────────────────────────────────────── */

/** Linear map: value in [in_min, in_max] → [out_min, out_max] */
static inline float map_range(float value,
                               float in_min,  float in_max,
                               float out_min, float out_max)
{
    if (in_max == in_min) return out_min;
    return out_min + (value - in_min) * (out_max - out_min) / (in_max - in_min);
}

/**
 * Convert a µs stick value [1000–2000] to a signed setpoint,
 * applying a deadband around centre (1500 µs).
 *
 * @param us        Raw µs from SBUS_ToMicros()
 * @param max_val   Maximum absolute output (±)
 * @return          Signed setpoint in [−max_val, +max_val]
 */
static float stick_to_setpoint(uint16_t us, float max_val)
{
    int16_t deviation = (int16_t)us - 1500;

    /* Apply deadband */
    if (deviation > 0 && deviation <  (int16_t)RC_DEADBAND_US) deviation = 0;
    if (deviation < 0 && deviation > -(int16_t)RC_DEADBAND_US) deviation = 0;

    return map_range((float)deviation, -500.0f, 500.0f, -max_val, max_val);
}

/** Clamp float to [lo, hi] */
static inline float clampf(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

/* ── Public API ────────────────────────────────────────────────────── */

void Attitude_Init(attitude_ctrl_t *ctrl)
{
    float dt = 1.0f / (float)MAIN_LOOP_RATE_HZ;   /* 0.001 s at 1 kHz */

    /* ── Outer angle loops (P-only — ki=0, kd=0) ──────────────────── */
    PID_Init(&ctrl->roll_angle,  PID_ROLL_ANGLE_KP,  0.0f, 0.0f, dt);
    PID_Init(&ctrl->pitch_angle, PID_PITCH_ANGLE_KP, 0.0f, 0.0f, dt);

    /* Clamp outer output to a sensible rate range: ±MAX_YAW_RATE_DPS
     * (reuse as a generic "max rate" bound — 200 deg/s is generous) */
    ctrl->roll_angle.output_limit  = 300.0f;  /* deg/s max rate setpoint */
    ctrl->pitch_angle.output_limit = 300.0f;

    /* ── Inner rate loops (full PID) ───────────────────────────────── */
    PID_Init(&ctrl->roll_rate,  PID_ROLL_RATE_KP,  PID_ROLL_RATE_KI,  PID_ROLL_RATE_KD,  dt);
    PID_Init(&ctrl->pitch_rate, PID_PITCH_RATE_KP, PID_PITCH_RATE_KI, PID_PITCH_RATE_KD, dt);
    PID_Init(&ctrl->yaw_rate,   PID_YAW_RATE_KP,   PID_YAW_RATE_KI,   PID_YAW_RATE_KD,   dt);

    /* Rate PID output clamped to ±500 µs (half the [1000–2000] range).
     * This prevents any single axis from saturating the mixer. */
    ctrl->roll_rate.output_limit  = 500.0f;
    ctrl->pitch_rate.output_limit = 500.0f;
    ctrl->yaw_rate.output_limit   = 500.0f;

    /* Anti-windup limits (integral × dt × ki should stay < output_limit) */
    ctrl->roll_rate.integral_limit  = 200.0f;
    ctrl->pitch_rate.integral_limit = 200.0f;
    ctrl->yaw_rate.integral_limit   = 150.0f;

    /* Derivative low-pass: alpha=0.15 → ~10 Hz cutoff at 1 kHz
     * Filters high-frequency gyro noise that would amplify D-term. */
    ctrl->roll_rate.d_filter_alpha  = 0.15f;
    ctrl->pitch_rate.d_filter_alpha = 0.15f;
    ctrl->yaw_rate.d_filter_alpha   = 0.15f;
}

uint8_t Attitude_Update(attitude_ctrl_t *ctrl)
{
    /* ── 1. Safety: check SBUS validity ───────────────────────────── */
    if (!SBUS_IsValid()) {
        Motor_StopAll();
        Attitude_Reset(ctrl);
        return 0;
    }

    /* ── 2. Read sensors ───────────────────────────────────────────── */
    const ahrs_state_t *ahrs = AHRS_GetState();

    imu_data_t imu;
    IMU_Read(&imu);
    IMU_ApplyCalibration(&imu);

    /* ── 3. Decode RC sticks ───────────────────────────────────────── */
    const sbus_data_t *rc = SBUS_GetData();

    uint16_t roll_us     = SBUS_ToMicros(rc->channel[SBUS_CH_AILERON]);
    uint16_t pitch_us    = SBUS_ToMicros(rc->channel[SBUS_CH_ELEVATOR]);
    uint16_t throttle_us = SBUS_ToMicros(rc->channel[SBUS_CH_THROTTLE]);
    uint16_t yaw_us      = SBUS_ToMicros(rc->channel[SBUS_CH_RUDDER]);

    /* Clamp to valid µs range (defensive) */
    throttle_us = (uint16_t)clampf((float)throttle_us, 1000.0f, 2000.0f);

    /* Stick → setpoints */
    float roll_sp_deg  = stick_to_setpoint(roll_us,  MAX_ROLL_ANGLE_DEG);
    float pitch_sp_deg = stick_to_setpoint(pitch_us, MAX_PITCH_ANGLE_DEG);
    float yaw_rate_sp  = stick_to_setpoint(yaw_us,   MAX_YAW_RATE_DPS);

    /* ── 4. Ground detection — freeze integrators to prevent windup ── */
    uint8_t on_ground = (throttle_us < THROTTLE_IDLE_US) ? 1U : 0U;

    if (on_ground) {
        /* Freeze I-terms while on the ground (don't call PID — no windup) */
        Motor_StopAll();
        Attitude_Reset(ctrl);
        return 1;
    }

    /* ── 5. Outer loop: Angle P-controller ────────────────────────── */
    /*
     * NED convention: roll right = positive, pitch forward = positive.
     * The AHRS yaw is [0°, 360°]; we don't use it in the outer loop.
     * pitch from AHRS: positive = nose up (matches RC convention).
     */
    float roll_rate_sp  = PID_Compute(&ctrl->roll_angle,  roll_sp_deg,  ahrs->roll);
    float pitch_rate_sp = PID_Compute(&ctrl->pitch_angle, pitch_sp_deg, ahrs->pitch);

    /* ── 6. Inner loop: Rate PID ───────────────────────────────────── */
    /*
     * IMU gyro axes:
     *   gyro_x → roll rate  (deg/s, positive = right)
     *   gyro_y → pitch rate (deg/s, positive = forward/up)
     *   gyro_z → yaw rate   (deg/s, positive = right/CW)
     */
    float roll_out  = PID_Compute(&ctrl->roll_rate,  roll_rate_sp,  imu.gyro_x);
    float pitch_out = PID_Compute(&ctrl->pitch_rate, pitch_rate_sp, imu.gyro_y);
    float yaw_out   = PID_Compute(&ctrl->yaw_rate,   yaw_rate_sp,   imu.gyro_z);

    /* ── 7. Mix → motor outputs ────────────────────────────────────── */
    mixer_output_t mix;
    Mixer_Compute(throttle_us, roll_out, pitch_out, yaw_out, &mix);

    /* ── 8. Drive motors ───────────────────────────────────────────── */
    Motor_SetAll(mix.m1, mix.m2, mix.m3, mix.m4);

    return 1;
}

void Attitude_Reset(attitude_ctrl_t *ctrl)
{
    PID_Reset(&ctrl->roll_angle);
    PID_Reset(&ctrl->pitch_angle);
    PID_Reset(&ctrl->roll_rate);
    PID_Reset(&ctrl->pitch_rate);
    PID_Reset(&ctrl->yaw_rate);
}
