/**
 * @file    motor.c
 * @brief   ESC / Motor PWM driver — TIM3 CH1–CH4 @ 400 Hz
 *
 * ── Timer clock derivation ───────────────────────────────────────────
 *
 *   SYSCLK  = 168 MHz
 *   APB1 prescaler = ÷4  → PCLK1 = 42 MHz
 *   APB1 timer clock = PCLK1 × 2 = 84 MHz   (STM32 rule: ×2 when APB≠÷1)
 *
 *   We want:  counter resolution = 1 µs  (so CCR value == pulse_us directly)
 *             PWM period          = 2500 µs  (400 Hz)
 *
 *   PSC = (84_000_000 / 1_000_000) - 1 = 83    → f_cnt  = 1 MHz (1 µs/tick)
 *   ARR = (1_000_000  /       400) - 1 = 2499  → f_pwm  = 400 Hz
 *
 * ── GPIO Alternate Function ──────────────────────────────────────────
 *   PA6 → TIM3_CH1  AF2
 *   PA7 → TIM3_CH2  AF2
 *   PB0 → TIM3_CH3  AF2
 *   PB1 → TIM3_CH4  AF2
 *
 * ── PWM Mode 1 (OCxM = 110) ─────────────────────────────────────────
 *   Output HIGH while CNT < CCR, LOW otherwise (normal active-high PWM).
 *   CCR value = desired pulse width in µs (1000–2000 valid range).
 */

#include "drivers/motor.h"
#include "pin_config.h"

/* ── Private constants ───────────────────────────────────────────────
 * All derived from the system clock. Change only CLOCK_APB1_TIMER_HZ
 * if you ever alter the APB1 prescaler in system_clock.c.
 */
#define CLOCK_APB1_TIMER_HZ     84000000UL  /* TIM3 input clock        */
#define MOTOR_CNT_FREQ_HZ        1000000UL  /* counter frequency: 1 MHz */
#define MOTOR_PWM_FREQ_HZ             400UL /* PWM frequency: 400 Hz    */

#define TIM3_PSC  ((CLOCK_APB1_TIMER_HZ / MOTOR_CNT_FREQ_HZ) - 1U)  /* 83   */
#define TIM3_ARR  ((MOTOR_CNT_FREQ_HZ   / MOTOR_PWM_FREQ_HZ) - 1U)  /* 2499 */

/* ── Private state ───────────────────────────────────────────────────*/
static TIM_HandleTypeDef s_htim3;
static bool              s_armed = false;

/* Motor-index → HAL channel constant lookup */
static const uint32_t k_channel[MOTOR_COUNT] = {
    TIM_CHANNEL_1,  /* MOTOR_1 — PA6 */
    TIM_CHANNEL_2,  /* MOTOR_2 — PA7 */
    TIM_CHANNEL_3,  /* MOTOR_3 — PB0 */
    TIM_CHANNEL_4,  /* MOTOR_4 — PB1 */
};

/* ── Forward declarations ────────────────────────────────────────────*/
static void     motor_gpio_init(void);
static void     motor_tim3_init(void);
static void     motor_write_us(motor_id_t id, uint16_t us);
static uint16_t motor_clamp_us(uint16_t us);

extern void Error_Handler(void);

/* ══════════════════════════════════════════════════════════════════════
 *  Public API
 * ══════════════════════════════════════════════════════════════════════ */

void Motor_Init(void)
{
    motor_gpio_init();
    motor_tim3_init();

    /* Drive all channels to IDLE immediately so ESCs see a valid signal
     * from the moment power is applied — prevents ESC from entering
     * calibration mode or making a fault sound.                         */
    for (motor_id_t m = MOTOR_1; m < MOTOR_COUNT; m++)
    {
        motor_write_us(m, MOTOR_PWM_IDLE_US);
    }
}

/* ── Motor_Arm ───────────────────────────────────────────────────────
 *
 * Standard BLHeli / SimonK ESC arming protocol:
 *   1. Send a pulse BELOW the minimum throttle (ARM_US = 700 µs) while
 *      the ESC powers up — this tells the ESC "I am a valid FC".
 *   2. Hold that signal for ≥2 s until the ESC beeps once (armed).
 *   3. Switch to IDLE_US (1000 µs) — safe to fly from here.
 *
 * IMPORTANT: Motors must NOT spin during arming. If they do, the
 * arm pulse width is too high — reduce MOTOR_PWM_ARM_US.
 */
void Motor_Arm(void)
{
    /* Send arm signal to all ESCs */
    for (motor_id_t m = MOTOR_1; m < MOTOR_COUNT; m++)
    {
        motor_write_us(m, MOTOR_PWM_ARM_US);
    }

    /* Block 2 seconds — ESC internal sequencer needs this window */
    HAL_Delay(2000U);

    /* Bring all channels to minimum throttle (armed idle) */
    for (motor_id_t m = MOTOR_1; m < MOTOR_COUNT; m++)
    {
        motor_write_us(m, MOTOR_PWM_IDLE_US);
    }

    s_armed = true;
}

void Motor_Disarm(void)
{
    s_armed = false;

    for (motor_id_t m = MOTOR_1; m < MOTOR_COUNT; m++)
    {
        motor_write_us(m, MOTOR_PWM_IDLE_US);
    }
}

bool Motor_IsArmed(void)
{
    return s_armed;
}

/* ── Motor_SetPWM ────────────────────────────────────────────────────
 *
 * Set pulse width directly in microseconds.
 * Input is hard-clamped to [MIN_US … MAX_US] regardless of caller value.
 * Silently ignored while disarmed.
 */
void Motor_SetPWM(motor_id_t id, uint16_t pulse_us)
{
    if (!s_armed || id >= MOTOR_COUNT) return;
    motor_write_us(id, motor_clamp_us(pulse_us));
}

/* ── Motor_SetThrottle ───────────────────────────────────────────────
 *
 * Normalised interface: throttle 0.0 → 1000 µs, 1.0 → 2000 µs.
 * Input is clamped to [0.0, 1.0] before mapping.
 *
 * Linear mapping:
 *   pulse_us = MIN_US + throttle × (MAX_US - MIN_US)
 *            = 1000   + throttle × 1000
 */
void Motor_SetThrottle(motor_id_t id, float throttle)
{
    if (!s_armed || id >= MOTOR_COUNT) return;

    if (throttle < 0.0f) throttle = 0.0f;
    if (throttle > 1.0f) throttle = 1.0f;

    uint16_t us = (uint16_t)(MOTOR_PWM_MIN_US +
                  (uint32_t)(throttle * (float)(MOTOR_PWM_MAX_US - MOTOR_PWM_MIN_US)));

    motor_write_us(id, us);
}

void Motor_SetAll(uint16_t m1, uint16_t m2, uint16_t m3, uint16_t m4)
{
    Motor_SetPWM(MOTOR_1, m1);
    Motor_SetPWM(MOTOR_2, m2);
    Motor_SetPWM(MOTOR_3, m3);
    Motor_SetPWM(MOTOR_4, m4);
}

void Motor_StopAll(void)
{
    /* Bypass armed check — always safe to force-stop */
    for (motor_id_t m = MOTOR_1; m < MOTOR_COUNT; m++)
    {
        motor_write_us(m, MOTOR_PWM_IDLE_US);
    }
}

/* ── Motor_CalibrateESC ──────────────────────────────────────────────
 *
 * Run ONCE on a bench with NO PROPELLERS ATTACHED.
 * Teaches the ESC the full throttle range so it uses the entire
 * 1000–2000 µs window linearly.
 *
 * Sequence (works for BLHeli, SimonK, AM32):
 *   1. Full throttle (2000 µs) for 3 s — ESC remembers this as MAX
 *   2. Zero throttle (1000 µs) for 3 s — ESC remembers this as MIN
 *   3. ESC plays a melody: calibration saved.
 */
void Motor_CalibrateESC(void)
{
    /* Step 1: send maximum */
    for (motor_id_t m = MOTOR_1; m < MOTOR_COUNT; m++)
    {
        motor_write_us(m, MOTOR_PWM_MAX_US);
    }
    HAL_Delay(3000U);

    /* Step 2: send minimum */
    for (motor_id_t m = MOTOR_1; m < MOTOR_COUNT; m++)
    {
        motor_write_us(m, MOTOR_PWM_MIN_US);
    }
    HAL_Delay(3000U);

    /* Leave outputs at idle — safe default */
    Motor_StopAll();
}

/* ══════════════════════════════════════════════════════════════════════
 *  Private helpers
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Configure PA6, PA7 (GPIOA) and PB0, PB1 (GPIOB) as
 *         Alternate Function Push-Pull, no pull, high speed, AF2 (TIM3).
 */
static void motor_gpio_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_NOPULL;
    gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
    gpio.Alternate = GPIO_AF2_TIM3;   /* TIM3 alternate function = AF2 */

    /* PA6 (CH1) and PA7 (CH2) in one call */
    gpio.Pin = MOTOR_1_PIN | MOTOR_2_PIN;   /* GPIO_PIN_6 | GPIO_PIN_7 */
    HAL_GPIO_Init(GPIOA, &gpio);

    /* PB0 (CH3) and PB1 (CH4) in one call */
    gpio.Pin = MOTOR_3_PIN | MOTOR_4_PIN;   /* GPIO_PIN_0 | GPIO_PIN_1 */
    HAL_GPIO_Init(GPIOB, &gpio);
}

/**
 * @brief  Configure TIM3 for 400 Hz PWM on all four channels.
 *
 * Timer base:
 *   PSC = 83  → f_cnt = 84 MHz ÷ 84 = 1 MHz  (1 count = 1 µs)
 *   ARR = 2499 → period = 2500 µs = 400 Hz
 *
 * OC channels: PWM Mode 1, preload enabled, active-high polarity.
 */
static void motor_tim3_init(void)
{
    __HAL_RCC_TIM3_CLK_ENABLE();

    /* ── Timer base ── */
    s_htim3.Instance               = TIM3;
    s_htim3.Init.Prescaler         = TIM3_PSC;           /* 83   */
    s_htim3.Init.CounterMode       = TIM_COUNTERMODE_UP;
    s_htim3.Init.Period            = TIM3_ARR;           /* 2499 */
    s_htim3.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    s_htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

    if (HAL_TIM_PWM_Init(&s_htim3) != HAL_OK)
    {
        Error_Handler();
    }

    /* ── Output compare config (same for all 4 channels) ── */
    TIM_OC_InitTypeDef oc = {0};
    oc.OCMode       = TIM_OCMODE_PWM1;       /* HIGH while CNT < CCR  */
    oc.OCPolarity   = TIM_OCPOLARITY_HIGH;
    oc.OCFastMode   = TIM_OCFAST_DISABLE;
    oc.Pulse        = MOTOR_PWM_IDLE_US;     /* 1000 µs initial value */

    const uint32_t channels[MOTOR_COUNT] = {
        TIM_CHANNEL_1, TIM_CHANNEL_2, TIM_CHANNEL_3, TIM_CHANNEL_4
    };

    for (uint8_t i = 0; i < MOTOR_COUNT; i++)
    {
        if (HAL_TIM_PWM_ConfigChannel(&s_htim3, &oc, channels[i]) != HAL_OK)
        {
            Error_Handler();
        }
        /* Start PWM output — signal is live from this point */
        if (HAL_TIM_PWM_Start(&s_htim3, channels[i]) != HAL_OK)
        {
            Error_Handler();
        }
    }
}

/**
 * @brief  Write pulse width directly to a CCR register (no bounds check,
 *         no armed check). Used internally only.
 */
static void motor_write_us(motor_id_t id, uint16_t us)
{
    __HAL_TIM_SET_COMPARE(&s_htim3, k_channel[id], (uint32_t)us);
}

/**
 * @brief  Clamp a pulse width to the safe [MIN_US … MAX_US] window.
 */
static uint16_t motor_clamp_us(uint16_t us)
{
    if (us < MOTOR_PWM_MIN_US) return MOTOR_PWM_MIN_US;
    if (us > MOTOR_PWM_MAX_US) return MOTOR_PWM_MAX_US;
    return us;
}
