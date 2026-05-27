/**
 * @file    madgwick.c
 * @brief   Madgwick AHRS filter — 6-DOF and 9-DOF variants
 *          Reference: S. Madgwick (2010) "An efficient orientation filter
 *          for inertial and inertial/magnetic sensor arrays"
 *
 * Algorithm overview (6-DOF):
 *   q_dot = 0.5 * q ⊗ [0,gx,gy,gz]  (pure gyro integration)
 *         - beta * normalise(J^T * f_g)  (gradient correction toward gravity)
 *   q    += q_dot / sample_freq
 *   q     = normalise(q)
 *
 * On STM32F405 with FPU: use sqrtf() — hardware single-precision instruction,
 * faster than the software fast-inverse-sqrt trick.
 */

#include "ahrs/madgwick.h"
#include <math.h>

#define DEG_TO_RAD  (0.017453293f)   /* π / 180 */

/* -------------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------------- */

void Madgwick_Init(madgwick_t *f, float sample_freq, float beta)
{
    f->q0 = 1.0f; f->q1 = 0.0f; f->q2 = 0.0f; f->q3 = 0.0f;
    f->beta        = beta;
    f->sample_freq = sample_freq;
}

/**
 * 6-DOF update: gyroscope + accelerometer only (no magnetometer).
 *
 * @param gx/gy/gz  Gyro in deg/s  (from IMU driver)
 * @param ax/ay/az  Accel in m/s²  (from IMU driver)
 *
 * Gravity reference vector in NED world frame: g_w = [0, 0, 1] (normalised).
 * Objective function:  f_g = R(q)^T * g_w  −  a_meas
 * Gradient correction: ∇f = J_g^T × f_g
 */
void Madgwick_Update6DOF(madgwick_t *f,
                         float gx, float gy, float gz,
                         float ax, float ay, float az)
{
    float q0 = f->q0, q1 = f->q1, q2 = f->q2, q3 = f->q3;
    float recip_norm;
    float s0, s1, s2, s3;
    float qDot1, qDot2, qDot3, qDot4;

    /* Step 1 — convert gyro deg/s → rad/s */
    gx *= DEG_TO_RAD;
    gy *= DEG_TO_RAD;
    gz *= DEG_TO_RAD;

    /* Step 2 — quaternion rate from gyroscope: q_dot = 0.5 * q ⊗ [0,gx,gy,gz] */
    qDot1 = 0.5f * (-q1*gx - q2*gy - q3*gz);
    qDot2 = 0.5f * ( q0*gx + q2*gz - q3*gy);
    qDot3 = 0.5f * ( q0*gy - q1*gz + q3*gx);
    qDot4 = 0.5f * ( q0*gz + q1*gy - q2*gx);

    /* Step 3 — gradient feedback (skip during freefall when |a| ≈ 0) */
    float accel_norm_sq = ax*ax + ay*ay + az*az;
    if (accel_norm_sq > 0.0f) {

        /* Normalise accelerometer measurement */
        recip_norm = 1.0f / sqrtf(accel_norm_sq);
        ax *= recip_norm;
        ay *= recip_norm;
        az *= recip_norm;

        /* Objective function: rotated gravity vs measured acceleration
         *   f1 = 2(q1*q3 - q0*q2) - ax
         *   f2 = 2(q0*q1 + q2*q3) - ay
         *   f3 = 1 - 2(q1² + q2²)  - az
         */
        float f1 = 2.0f*(q1*q3 - q0*q2) - ax;
        float f2 = 2.0f*(q0*q1 + q2*q3) - ay;
        float f3 = 1.0f - 2.0f*(q1*q1 + q2*q2) - az;

        /* Gradient: ∇f = J_g^T × [f1, f2, f3]
         *   (derived from Jacobian of the rotated gravity vector) */
        s0 = -2.0f*q2*f1 + 2.0f*q1*f2;
        s1 =  2.0f*q3*f1 + 2.0f*q0*f2 - 4.0f*q1*f3;
        s2 = -2.0f*q0*f1 + 2.0f*q3*f2 - 4.0f*q2*f3;
        s3 =  2.0f*q1*f1 + 2.0f*q2*f2;

        /* Normalise gradient, then subtract beta-weighted correction from q_dot */
        recip_norm = 1.0f / sqrtf(s0*s0 + s1*s1 + s2*s2 + s3*s3);
        qDot1 -= f->beta * s0 * recip_norm;
        qDot2 -= f->beta * s1 * recip_norm;
        qDot3 -= f->beta * s2 * recip_norm;
        qDot4 -= f->beta * s3 * recip_norm;
    }

    /* Step 4 — integrate: q += q_dot * dt */
    float dt = 1.0f / f->sample_freq;
    q0 += qDot1 * dt;
    q1 += qDot2 * dt;
    q2 += qDot3 * dt;
    q3 += qDot4 * dt;

    /* Step 5 — normalise quaternion */
    recip_norm = 1.0f / sqrtf(q0*q0 + q1*q1 + q2*q2 + q3*q3);
    f->q0 = q0 * recip_norm;
    f->q1 = q1 * recip_norm;
    f->q2 = q2 * recip_norm;
    f->q3 = q3 * recip_norm;
}

/**
 * 9-DOF update: gyroscope + accelerometer + magnetometer.
 * Hardware has no magnetometer — calls 6-DOF and ignores mag input.
 * Provided for API completeness if a mag sensor is added later.
 */
void Madgwick_Update9DOF(madgwick_t *f,
                         float gx, float gy, float gz,
                         float ax, float ay, float az,
                         float mx, float my, float mz)
{
    /* No magnetometer in hardware — fall through to gyro+accel only */
    (void)mx; (void)my; (void)mz;
    Madgwick_Update6DOF(f, gx, gy, gz, ax, ay, az);
}

/**
 * Convert quaternion to ZYX Euler angles (radians).
 * roll ∈ [-π, π], pitch ∈ [-π/2, π/2], yaw ∈ [-π, π]
 * Caller should map yaw to [0°, 360°] for display (ahrs.c does this).
 */
void Madgwick_GetEuler(const madgwick_t *f,
                       float *roll, float *pitch, float *yaw)
{
    *roll  = atan2f(2.0f*(f->q0*f->q1 + f->q2*f->q3),
                    1.0f - 2.0f*(f->q1*f->q1 + f->q2*f->q2));
    *pitch = asinf (2.0f*(f->q0*f->q2 - f->q3*f->q1));
    *yaw   = atan2f(2.0f*(f->q0*f->q3 + f->q1*f->q2),
                    1.0f - 2.0f*(f->q2*f->q2 + f->q3*f->q3));
}
