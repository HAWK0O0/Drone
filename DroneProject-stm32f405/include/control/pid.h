/**
 * @file    pid.h
 * @brief   Generic PID controller with anti-windup and derivative filtering
 */

#ifndef FC_PID_H
#define FC_PID_H

#include <stdint.h>

typedef struct {
    float kp;
    float ki;
    float kd;

    float integral;
    float prev_error;
    float prev_derivative;

    float integral_limit;   /* anti-windup clamp */
    float output_limit;     /* output clamp (0 = disabled) */
    float d_filter_alpha;   /* low-pass filter on derivative (0–1) */

    float dt;               /* seconds — set from sample rate */
} pid_ctrl_t;

/** Initialise a PID controller */
void PID_Init(pid_ctrl_t *pid, float kp, float ki, float kd, float dt);

/** Compute PID output for a given setpoint and measured value */
float PID_Compute(pid_ctrl_t *pid, float setpoint, float measured);

/** Reset integral and previous error (e.g., on disarm) */
void PID_Reset(pid_ctrl_t *pid);

/** Update gains at runtime (e.g., from parameter MAVLink message) */
void PID_SetGains(pid_ctrl_t *pid, float kp, float ki, float kd);

#endif /* FC_PID_H */
