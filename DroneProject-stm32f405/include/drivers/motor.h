/**
 * @file    motor.h
 * @brief   ESC / Motor PWM driver — TIM3 CH1–CH4 @ 400 Hz
 *
 * Hardware mapping (X-frame quadcopter):
 * ┌──────────────────────────────────────────────────┐
 * │  Motor │ Pin  │ Timer Channel │ Frame position   │
 * │   M1   │ PA6  │ TIM3_CH1      │ Front-Left  (CW) │
 * │   M2   │ PA7  │ TIM3_CH2      │ Front-Right(CCW) │
 * │   M3   │ PB0  │ TIM3_CH3      │ Rear-Right  (CW) │
 * │   M4   │ PB1  │ TIM3_CH4      │ Rear-Left  (CCW) │
 * └──────────────────────────────────────────────────┘
 *
 * PWM spec (standard ESC):
 *   Period  : 2500 µs  (400 Hz)
 *   Min     : 1000 µs  (zero throttle / ESC armed idle)
 *   Max     : 2000 µs  (full throttle)
 *   Arm seq : ARM_US for ≥2 s → ESC beeps → armed
 *
 * Typical call order in main():
 *   Motor_Init();
 *   Motor_Arm();      // blocks ~2 s while ESCs initialise
 *   ...flight loop...
 *   Motor_SetAll(m1, m2, m3, m4);
 *   Motor_Disarm();
 */

#ifndef FC_MOTOR_H
#define FC_MOTOR_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* ── PWM pulse-width constants (microseconds) ─────────────────────── */
#define MOTOR_PWM_MIN_US    1000U   /**< Minimum throttle (ESC idle)    */
#define MOTOR_PWM_MAX_US    2000U   /**< Maximum throttle               */
#define MOTOR_PWM_IDLE_US   1000U   /**< Safe value after arm           */
#define MOTOR_PWM_ARM_US     700U   /**< Below-minimum arm signal       */

/* ── Motor index ──────────────────────────────────────────────────── */
typedef enum {
    MOTOR_1 = 0,  /**< Front-Left  — PA6  TIM3_CH1 CW  */
    MOTOR_2,      /**< Front-Right — PA7  TIM3_CH2 CCW */
    MOTOR_3,      /**< Rear-Right  — PB0  TIM3_CH3 CW  */
    MOTOR_4,      /**< Rear-Left   — PB1  TIM3_CH4 CCW */
    MOTOR_COUNT
} motor_id_t;

/* ──────────────────────────────────────────────────────────────────── */
/*  Lifecycle                                                           */
/* ──────────────────────────────────────────────────────────────────── */

/**
 * @brief  Initialise TIM3 GPIO and PWM hardware.
 *         All channels output MOTOR_PWM_IDLE_US (1000 µs) immediately.
 *         Does NOT arm the ESCs — call Motor_Arm() next.
 */
void Motor_Init(void);

/**
 * @brief  ESC arming sequence: sends MOTOR_PWM_ARM_US for 2 s.
 *         Blocks until complete. ESCs beep when armed.
 *         After this call Motor_IsArmed() returns true.
 */
void Motor_Arm(void);

/**
 * @brief  Disarm: set all channels to MOTOR_PWM_IDLE_US and clear
 *         the armed flag. Safe to call at any time.
 */
void Motor_Disarm(void);

/**
 * @brief  Returns true if Motor_Arm() has been called successfully.
 */
bool Motor_IsArmed(void);

/* ──────────────────────────────────────────────────────────────────── */
/*  Throttle control                                                    */
/* ──────────────────────────────────────────────────────────────────── */

/**
 * @brief  Set PWM pulse width for one motor.
 * @param  id       MOTOR_1 … MOTOR_4
 * @param  pulse_us Clamped to [MOTOR_PWM_MIN_US, MOTOR_PWM_MAX_US].
 * @note   No-op if not armed.
 */
void Motor_SetPWM(motor_id_t id, uint16_t pulse_us);

/**
 * @brief  Set normalised throttle [0.0, 1.0] for one motor.
 *         0.0 → 1000 µs, 1.0 → 2000 µs. Input is clamped.
 * @note   No-op if not armed.
 */
void Motor_SetThrottle(motor_id_t id, float throttle);

/**
 * @brief  Set all four motors in one call (pulse widths in µs).
 * @note   No-op if not armed.
 */
void Motor_SetAll(uint16_t m1, uint16_t m2, uint16_t m3, uint16_t m4);

/**
 * @brief  Set all four motors to MOTOR_PWM_IDLE_US (safe stop).
 *         Works whether armed or not.
 */
void Motor_StopAll(void);

/* ──────────────────────────────────────────────────────────────────── */
/*  ESC calibration (run once, drone on bench, no props!)              */
/* ──────────────────────────────────────────────────────────────────── */

/**
 * @brief  Standard ESC calibration: MAX for 3 s → MIN for 3 s.
 *         Drone MUST be on a bench with propellers removed.
 *         Blocks for ~6 s total.
 */
void Motor_CalibrateESC(void);

#endif /* FC_MOTOR_H */
