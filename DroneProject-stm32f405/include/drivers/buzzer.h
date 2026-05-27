/**
 * @file    buzzer.h
 * @brief   Buzzer tone driver — TIM12 CH1 (PB14)
 */

#ifndef FC_BUZZER_H
#define FC_BUZZER_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

typedef enum {
    BUZZ_BOOT = 0,       /* single short beep on startup   */
    BUZZ_ARMED,          /* double beep — armed            */
    BUZZ_DISARMED,       /* single long beep — disarmed    */
    BUZZ_FAILSAFE,       /* rapid beeps — failsafe         */
    BUZZ_LOW_BATTERY,    /* slow double beep — low battery */
    BUZZ_ERROR,          /* long tone — critical error     */
    BUZZ_SD_READY        /* triple beep — SD card OK       */
} buzzer_pattern_t;

/** Initialise TIM12 CH1 for buzzer PWM (2.8 kHz tone) */
void Buzzer_Init(void);

/** Play a predefined beep pattern (non-blocking) */
void Buzzer_Play(buzzer_pattern_t pattern);

/** Update buzzer state machine — call every 10 ms */
void Buzzer_Update(void);

/** Stop any active tone immediately */
void Buzzer_Stop(void);

#endif /* FC_BUZZER_H */
