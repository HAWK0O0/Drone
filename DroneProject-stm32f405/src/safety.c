/**
 * @file    safety.c
 * @brief   Arming state machine, IWDG watchdog, failsafe
 *
 * IWDG math:
 *   LSI ≈ 32 000 Hz;  IWDG_PRESCALER_32 → tick = 1 000 Hz
 *   Reload = 499 → timeout ≈ 500 ms
 *   If main loop hangs (e.g. I2C lockup), watchdog resets the MCU.
 *
 * Arm switch:
 *   SBUS CH4 > 1700 µs → arm request
 *   SBUS CH4 < 1300 µs → disarm request
 */

#include "safety.h"
#include "drivers/motor.h"
#include "drivers/sbus.h"
#include "drivers/imu.h"
#include "drivers/buzzer.h"
#include "config.h"

/* ── RC channel indices (0-based) ─────────────────────────────────── */
#define RC_THROTTLE_CH   2U
#define RC_ARM_CH        4U

/* Throttle must be below this (µs) before arming is allowed */
#define THROTTLE_ARM_MAX_US   1100U
/* Arm switch thresholds */
#define ARM_SWITCH_ON_US      1700U
#define ARM_SWITCH_OFF_US     1300U

/* Time from boot before AHRS is considered converged (ms) */
#define AHRS_SETTLE_MS        2000U

static arm_state_t         s_state     = ARM_STATE_DISARMED;
static IWDG_HandleTypeDef  s_hiwdg;
static uint32_t            s_init_tick = 0U;

/* ── Private helpers ─────────────────────────────────────────────── */

/** Simple convergence proxy — 2 s after boot at 1 kHz */
static uint8_t ahrs_ready(void)
{
    return (HAL_GetTick() - s_init_tick) >= AHRS_SETTLE_MS;
}

/* ── Public API ──────────────────────────────────────────────────── */

void Safety_Init(void)
{
    s_state     = ARM_STATE_DISARMED;
    s_init_tick = HAL_GetTick();

    /* IWDG: LSI / PRESCALER_32 → 1 kHz tick; Reload=499 → ~500 ms timeout */
    s_hiwdg.Instance       = IWDG;
    s_hiwdg.Init.Prescaler = IWDG_PRESCALER_32;
    s_hiwdg.Init.Reload    = 499U;
    HAL_IWDG_Init(&s_hiwdg);
}

void Safety_Refresh(void)
{
    HAL_IWDG_Refresh(&s_hiwdg);
}

uint8_t Safety_PreArmCheck(void)
{
    /* 1. RC link must be valid */
    if (!SBUS_IsValid()) return 0U;

    /* 2. Throttle must be at minimum before arming */
    const sbus_data_t *rc = SBUS_GetData();
    uint16_t thr_us = SBUS_ToMicros(rc->channel[RC_THROTTLE_CH]);
    if (thr_us >= THROTTLE_ARM_MAX_US) return 0U;

    /* 3. AHRS must have settled */
    if (!ahrs_ready()) return 0U;

    return 1U;
}

void Safety_RequestArm(void)
{
    if (s_state == ARM_STATE_DISARMED && Safety_PreArmCheck()) {
        Motor_Arm();                /* 2-second ESC arming sequence */
        s_state = ARM_STATE_ARMED;
    }
}

void Safety_RequestDisarm(void)
{
    Motor_StopAll();
    Motor_Disarm();
    s_state = ARM_STATE_DISARMED;
}

void Safety_Failsafe(void)
{
    Motor_StopAll();
    Motor_Disarm();
    s_state = ARM_STATE_FAILSAFE;
}

arm_state_t Safety_GetState(void)
{
    return s_state;
}

uint8_t Safety_IsArmed(void)
{
    return (s_state == ARM_STATE_ARMED) ? 1U : 0U;
}

void Safety_Update(void)
{
    static arm_state_t s_prev = ARM_STATE_DISARMED;

    /* ── SBUS failsafe check ──────────────────────────────────────── */
    if (!SBUS_IsValid()) {
        if (s_state != ARM_STATE_FAILSAFE) {
            Safety_Failsafe();
        }
        goto check_transition;
    }

    /* SBUS just restored → back to DISARMED for a clean re-arm cycle */
    if (s_state == ARM_STATE_FAILSAFE) {
        Motor_StopAll();
        s_state = ARM_STATE_DISARMED;
        goto check_transition;
    }

    /* ── Arm switch polling ───────────────────────────────────────── */
    {
        const sbus_data_t *rc = SBUS_GetData();
        uint16_t arm_us = SBUS_ToMicros(rc->channel[RC_ARM_CH]);

        if ((arm_us > ARM_SWITCH_ON_US) && (s_state == ARM_STATE_DISARMED)) {
            Safety_RequestArm();
        } else if ((arm_us < ARM_SWITCH_OFF_US) && (s_state == ARM_STATE_ARMED)) {
            Safety_RequestDisarm();
        }
    }

check_transition:
    /* ── Buzzer events on state change ───────────────────────────── */
    if (s_state != s_prev) {
        if      (s_state == ARM_STATE_ARMED)    Buzzer_Play(BUZZ_ARMED);
        else if (s_state == ARM_STATE_DISARMED) Buzzer_Play(BUZZ_DISARMED);
        else if (s_state == ARM_STATE_FAILSAFE) Buzzer_Play(BUZZ_FAILSAFE);
        s_prev = s_state;
    }
}
