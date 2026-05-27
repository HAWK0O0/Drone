/**
 * @file    ahrs.h
 * @brief   Attitude & Heading Reference System — wrapper around Madgwick
 */

#ifndef FC_AHRS_H
#define FC_AHRS_H

#include <stdint.h>

typedef struct {
    float roll;    /* degrees, positive = right roll  */
    float pitch;   /* degrees, positive = nose up     */
    float yaw;     /* degrees, 0–360 clockwise from N */
    float q0, q1, q2, q3;  /* underlying quaternion  */
} ahrs_state_t;

/** Initialise AHRS (calls Madgwick_Init internally) */
void AHRS_Init(void);

/** Update AHRS — call at ATTITUDE_RATE_HZ (1 kHz) */
void AHRS_Update(void);

/** Get pointer to latest attitude state */
const ahrs_state_t *AHRS_GetState(void);

/** Returns 1 once the attitude estimate has converged */
uint8_t AHRS_IsConverged(void);

/** Reset filter (e.g., after hard crash) */
void AHRS_Reset(void);

#endif /* FC_AHRS_H */
