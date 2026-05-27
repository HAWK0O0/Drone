/**
 * @file    led.c
 * @brief   Status LED driver — PB12 (Blue), PB13 (Red)
 */

#include "drivers/led.h"
#include "pin_config.h"

typedef struct {
    GPIO_TypeDef  *port;
    uint16_t       pin;
    led_pattern_t  pattern;
    uint32_t       next_tick;
    uint8_t        state;
    uint8_t        blink_count;
} led_ctx_t;

static led_ctx_t s_leds[2] = {
    [LED_BLUE] = { .port = LED_BLUE_PORT, .pin = LED_BLUE_PIN },
    [LED_RED]  = { .port = LED_RED_PORT,  .pin = LED_RED_PIN  },
};

void LED_Init(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin   = LED_BLUE_PIN | LED_RED_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &gpio);

    HAL_GPIO_WritePin(GPIOB, LED_BLUE_PIN | LED_RED_PIN, GPIO_PIN_RESET);
}

void LED_SetPattern(led_id_t led, led_pattern_t pattern)
{
    s_leds[led].pattern    = pattern;
    s_leds[led].blink_count = 0;
    s_leds[led].next_tick   = 0;
}

void LED_Update(void)
{
    uint32_t now = HAL_GetTick();
    for (int i = 0; i < 2; i++) {
        led_ctx_t *l = &s_leds[i];
        if (now < l->next_tick) continue;
        switch (l->pattern) {
            case LED_PATTERN_OFF:
                HAL_GPIO_WritePin(l->port, l->pin, GPIO_PIN_RESET);
                break;
            case LED_PATTERN_ON:
                HAL_GPIO_WritePin(l->port, l->pin, GPIO_PIN_SET);
                break;
            case LED_PATTERN_BLINK_SLOW:
                HAL_GPIO_TogglePin(l->port, l->pin);
                l->next_tick = now + 500U;
                break;
            case LED_PATTERN_BLINK_FAST:
                HAL_GPIO_TogglePin(l->port, l->pin);
                l->next_tick = now + 125U;
                break;
            default:
                break;
        }
    }
}

void LED_On(led_id_t led)
{
    HAL_GPIO_WritePin(s_leds[led].port, s_leds[led].pin, GPIO_PIN_SET);
}

void LED_Off(led_id_t led)
{
    HAL_GPIO_WritePin(s_leds[led].port, s_leds[led].pin, GPIO_PIN_RESET);
}

void LED_Toggle(led_id_t led)
{
    HAL_GPIO_TogglePin(s_leds[led].port, s_leds[led].pin);
}
