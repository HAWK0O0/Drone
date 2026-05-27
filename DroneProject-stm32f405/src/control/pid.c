/**
 * @file    pid.c
 * @brief   Generic PID controller — anti-windup, derivative low-pass filter
 */

#include "control/pid.h"
#include <string.h>

void PID_Init(pid_ctrl_t *pid, float kp, float ki, float kd, float dt)
{
    memset(pid, 0, sizeof(*pid));
    pid->kp             = kp;
    pid->ki             = ki;
    pid->kd             = kd;
    pid->dt             = dt;
    pid->integral_limit = 400.0f;  /* default anti-windup limit */
    pid->d_filter_alpha = 0.1f;    /* derivative low-pass coefficient */
}

float PID_Compute(pid_ctrl_t *pid, float setpoint, float measured)
{
    float error = setpoint - measured;

    /* Proportional */
    float p = pid->kp * error;

    /* Integral with anti-windup */
    pid->integral += error * pid->dt;
    if      (pid->integral >  pid->integral_limit) pid->integral =  pid->integral_limit;
    else if (pid->integral < -pid->integral_limit) pid->integral = -pid->integral_limit;
    float i = pid->ki * pid->integral;

    /* Derivative with low-pass filter */
    float raw_d = (error - pid->prev_error) / pid->dt;
    pid->prev_derivative = pid->d_filter_alpha * raw_d +
                           (1.0f - pid->d_filter_alpha) * pid->prev_derivative;
    float d = pid->kd * pid->prev_derivative;

    pid->prev_error = error;

    float output = p + i + d;

    /* Output clamp */
    if (pid->output_limit > 0.0f) {
        if      (output >  pid->output_limit) output =  pid->output_limit;
        else if (output < -pid->output_limit) output = -pid->output_limit;
    }

    return output;
}

void PID_Reset(pid_ctrl_t *pid)
{
    pid->integral        = 0.0f;
    pid->prev_error      = 0.0f;
    pid->prev_derivative = 0.0f;
}

void PID_SetGains(pid_ctrl_t *pid, float kp, float ki, float kd)
{
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    PID_Reset(pid);
}
