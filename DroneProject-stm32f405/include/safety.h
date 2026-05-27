/**
 * @file    safety.h
 * @brief   Arming state machine, watchdog, and failsafe logic
 */

#ifndef FC_SAFETY_H
#define FC_SAFETY_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

typedef enum {
    ARM_STATE_DISARMED = 0,
    ARM_STATE_PRE_ARM_CHECK,
    ARM_STATE_ARMED,
    ARM_STATE_FAILSAFE,
    ARM_STATE_ERROR
} arm_state_t;

/** Initialise IWDG watchdog and safety subsystem */
void Safety_Init(void);

/** Must be called every main loop iteration to refresh IWDG */
void Safety_Refresh(void);

/** Run pre-arm checks (IMU calibrated, RC OK, throttle low, etc.)
 *  @return 1 if all checks pass */
uint8_t Safety_PreArmCheck(void);

/** Request arm / disarm transition */
void Safety_RequestArm(void);
void Safety_RequestDisarm(void);

/** Immediately disarm and enter failsafe */
void Safety_Failsafe(void);

/** Get current arming state */
arm_state_t Safety_GetState(void);

/** Returns 1 if motors are allowed to spin */
uint8_t Safety_IsArmed(void);

/**
 * @brief  Poll arm switch, check failsafe, emit buzzer events on transitions.
 *         Call at 10–100 Hz from main loop (or scheduler).
 */
void Safety_Update(void);

#endif /* FC_SAFETY_H */
