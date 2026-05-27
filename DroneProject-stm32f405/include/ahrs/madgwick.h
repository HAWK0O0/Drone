/**
 * @file    madgwick.h
 * @brief   Madgwick AHRS filter — 6-DOF (IMU-only) + 9-DOF (with mag)
 *          Reference: Madgwick (2010) "An efficient orientation filter..."
 */

#ifndef FC_MADGWICK_H
#define FC_MADGWICK_H

#include <stdint.h>

typedef struct {
    float q0, q1, q2, q3;   /* unit quaternion */
    float beta;              /* filter gain (see config.h MADGWICK_BETA) */
    float sample_freq;       /* Hz */
} madgwick_t;

/** Initialise filter with given sample frequency and beta gain */
void Madgwick_Init(madgwick_t *f, float sample_freq, float beta);

/** Update with 6-DOF IMU data (no magnetometer) */
void Madgwick_Update6DOF(madgwick_t *f,
                         float gx, float gy, float gz,   /* deg/s */
                         float ax, float ay, float az);  /* m/s²  */

/** Update with 9-DOF data (IMU + magnetometer) */
void Madgwick_Update9DOF(madgwick_t *f,
                         float gx, float gy, float gz,
                         float ax, float ay, float az,
                         float mx, float my, float mz);  /* µT     */

/** Extract Euler angles from quaternion (radians) */
void Madgwick_GetEuler(const madgwick_t *f,
                       float *roll, float *pitch, float *yaw);

#endif /* FC_MADGWICK_H */
