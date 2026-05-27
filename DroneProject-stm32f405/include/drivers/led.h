/**
 * @file    led.h
 * @brief   Status LED driver — PB12 (Blue), PB13 (Red)
 */

#ifndef FC_LED_H
#define FC_LED_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

typedef enum {
    LED_BLUE = 0,
    LED_RED
} led_id_t;

typedef enum {
    LED_PATTERN_OFF = 0,
    LED_PATTERN_ON,
    LED_PATTERN_BLINK_SLOW,   /* 1 Hz */
    LED_PATTERN_BLINK_FAST,   /* 4 Hz */
    LED_PATTERN_DOUBLE_BLINK,
    LED_PATTERN_SOS
} led_pattern_t;

/** Initialise GPIO for LEDs */
void LED_Init(void);

/** Set LED blink pattern */
void LED_SetPattern(led_id_t led, led_pattern_t pattern);

/** Update blink state machines — call every 10 ms */
void LED_Update(void);

/** Directly set LED on/off */
void LED_On(led_id_t led);
void LED_Off(led_id_t led);
void LED_Toggle(led_id_t led);

#endif /* FC_LED_H */
