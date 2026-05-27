/**
 * @file    buzzer.c
 * @brief   Buzzer tone driver — TIM12 CH1 (PB14) @ 2.8 kHz
 *
 * Timer math:
 *   APB1 timer clock = 84 MHz  (42 MHz × 2 — APB prescaler rule)
 *   PSC = 0   → counter ticks at 84 MHz
 *   ARR = 29999 → f_pwm = 84 000 000 / 30 000 = 2 800 Hz
 *   CCR1 = 14999 → 50 % duty cycle
 */

#include "drivers/buzzer.h"
#include "pin_config.h"

#define BUZZER_FREQ_HZ      2800U
#define BUZZER_TIMER_CLK    84000000UL
#define BUZZER_ARR          ((BUZZER_TIMER_CLK / BUZZER_FREQ_HZ) - 1UL)  /* 29999 */
#define BUZZER_CCR          (BUZZER_ARR / 2UL)                            /* 14999 */

typedef struct { uint16_t on_ms; uint16_t off_ms; uint8_t count; } beep_t;

static const beep_t k_patterns[] = {
    [BUZZ_BOOT]        = { 80,   0, 1},   /* single short beep at power-on        */
    [BUZZ_ARMED]       = {100,  80, 2},   /* double beep — armed                  */
    [BUZZ_DISARMED]    = {400,   0, 1},   /* single long beep — disarmed          */
    [BUZZ_FAILSAFE]    = { 80,  80, 6},   /* rapid 6×  beep — RC signal lost      */
    [BUZZ_LOW_BATTERY] = {200, 500, 3},   /* slow triple beep — battery warning   */
    [BUZZ_ERROR]       = {800, 200, 1},   /* long tone  — critical fault          */
    [BUZZ_SD_READY]    = {100,  80, 3},   /* triple beep — SD card mounted        */
};

static TIM_HandleTypeDef s_htim12;
static buzzer_pattern_t  s_pattern     = BUZZ_BOOT;
static uint8_t           s_beeps_left  = 0;
static uint32_t          s_next_tick   = 0;
static uint8_t           s_tone_on     = 0;

/* ── Private helpers ─────────────────────────────────────────────── */

static void buzzer_tone_start(void)
{
    HAL_TIM_PWM_Start(&s_htim12, TIM_CHANNEL_1);
    s_tone_on = 1;
}

static void buzzer_tone_stop(void)
{
    HAL_TIM_PWM_Stop(&s_htim12, TIM_CHANNEL_1);
    s_tone_on = 0;
}

/* ── Public API ──────────────────────────────────────────────────── */

void Buzzer_Init(void)
{
    /* 1. Enable clocks */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_TIM12_CLK_ENABLE();

    /* 2. PB14 → AF9 (TIM12_CH1) */
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin       = BUZZER_PIN;      /* GPIO_PIN_14 */
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_NOPULL;
    gpio.Speed     = GPIO_SPEED_FREQ_LOW;
    gpio.Alternate = GPIO_AF9_TIM12;
    HAL_GPIO_Init(BUZZER_PORT, &gpio);  /* GPIOB */

    /* 3. TIM12 base: PSC=0, ARR=29999 → 2800 Hz @ 84 MHz */
    s_htim12.Instance               = TIM12;
    s_htim12.Init.Prescaler         = 0;
    s_htim12.Init.CounterMode       = TIM_COUNTERMODE_UP;
    s_htim12.Init.Period            = (uint32_t)BUZZER_ARR;
    s_htim12.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    s_htim12.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_PWM_Init(&s_htim12);

    /* 4. TIM12 CH1 PWM mode 1, 50 % duty */
    TIM_OC_InitTypeDef oc = {0};
    oc.OCMode       = TIM_OCMODE_PWM1;
    oc.Pulse        = (uint32_t)BUZZER_CCR;
    oc.OCPolarity   = TIM_OCPOLARITY_HIGH;
    oc.OCFastMode   = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&s_htim12, &oc, TIM_CHANNEL_1);
    /* Timer is configured but NOT started — Buzzer_Play() controls that */
}

void Buzzer_Play(buzzer_pattern_t pattern)
{
    buzzer_tone_stop();            /* interrupt any active tone immediately */
    s_pattern    = pattern;
    s_beeps_left = k_patterns[pattern].count;
    s_next_tick  = HAL_GetTick();  /* start immediately */
    s_tone_on    = 0;
}

void Buzzer_Update(void)
{
    if (s_beeps_left == 0) return;

    uint32_t now = HAL_GetTick();
    if (now < s_next_tick) return;

    if (!s_tone_on) {
        /* Start beep */
        buzzer_tone_start();
        s_next_tick = now + k_patterns[s_pattern].on_ms;
    } else {
        /* End beep */
        buzzer_tone_stop();
        s_beeps_left--;
        s_next_tick = now + k_patterns[s_pattern].off_ms;
    }
}

void Buzzer_Stop(void)
{
    s_beeps_left = 0;
    buzzer_tone_stop();
}
