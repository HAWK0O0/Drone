/**
 * @file    system_clock.c
 * @brief   System clock + FPU configuration for STM32F405RGT6
 *
 *  Target: SYSCLK = 168 MHz
 *  Source: HSE 8 MHz (WeAct-STM32F4 Core Board V1.1)
 *
 *  PLL chain:
 *    HSE(8) ÷ PLLM(8)  =    1 MHz  VCO input  (must be 1–2 MHz)
 *    1 MHz  × PLLN(336) =  336 MHz  VCO output (must be 192–432 MHz)
 *    336    ÷ PLLP(2)   =  168 MHz  SYSCLK
 *    336    ÷ PLLQ(7)   ≈   48 MHz  USB OTG FS / SDIO / RNG
 *
 *  Bus prescalers:
 *    AHB  ÷1  → HCLK  = 168 MHz  (CPU, DMA, peripherals on AHB)
 *    APB1 ÷4  → PCLK1 =  42 MHz  (I2C, USART2/3, SPI2/3, TIM2-7,12-14)
 *    APB2 ÷2  → PCLK2 =  84 MHz  (USART1/6, SPI1, ADC, TIM1/8-11)
 *
 *  Flash:  5 wait-states, instruction + data cache, prefetch enabled.
 *  FPU:    CP10 & CP11 full access before any floating-point code runs.
 */

#include "system_clock.h"

/* Forward declaration — implement in main.c */
void Error_Handler(void);

/* ------------------------------------------------------------------ */

void SystemClock_Config(void)
{
    /* ── 1. Enable Cortex-M4 FPU ──────────────────────────────────
     *
     * CPACR (Coprocessor Access Control Register) lives at 0xE000ED88.
     * Bits [23:22] control CP11 (FPU high half) and [21:20] CP10 (FPU).
     * Value 0b11 = full access from privileged AND unprivileged code.
     *
     * This must be done BEFORE any floating-point instruction executes.
     * Without it the CPU raises a UsageFault on the first FP operation.
     */
    SCB->CPACR |= ((3UL << (10U * 2U)) |   /* CP10: full access */
                   (3UL << (11U * 2U)));    /* CP11: full access */
    __DSB();   /* data sync barrier — ensure write is visible  */
    __ISB();   /* instruction sync — flush the pipeline        */

    /* ── 2. Enable PWR clock & set voltage scaling ────────────────
     *
     * Scale 1 = 1.8 V internal regulator core voltage.
     * Required for SYSCLK > 144 MHz; mandatory at 168 MHz.
     * Without it the maximum safe frequency is only 120 MHz.
     */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /* ── 3. Configure HSE oscillator + main PLL ───────────────────
     *
     * All PLL input/output constraints (from RM0090 reference manual):
     *   VCO input frequency  : 1–2 MHz      → we use 1 MHz
     *   VCO output frequency : 192–432 MHz  → we use 336 MHz
     *   PLLP output          : ≤ 168 MHz    → 168 MHz OK
     *   PLLQ output          : ≥ 48 MHz for USB → 48 MHz OK
     */
    RCC_OscInitTypeDef osc_cfg = {0};

    osc_cfg.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc_cfg.HSEState       = RCC_HSE_ON;          /* external crystal */
    osc_cfg.PLL.PLLState   = RCC_PLL_ON;
    osc_cfg.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    osc_cfg.PLL.PLLM       = CLOCK_PLL_M;         /* ÷8  → 1 MHz      */
    osc_cfg.PLL.PLLN       = CLOCK_PLL_N;         /* ×336 → 336 MHz   */
    osc_cfg.PLL.PLLP       = RCC_PLLP_DIV2;       /* ÷2  → 168 MHz    */
    osc_cfg.PLL.PLLQ       = CLOCK_PLL_Q;         /* ÷7  → 48 MHz     */

    if (HAL_RCC_OscConfig(&osc_cfg) != HAL_OK)
    {
        Error_Handler();
    }

    /* ── 4. Configure bus clock prescalers & select PLL as SYSCLK ─
     *
     * Flash latency rule (RM0090 Table 10, Vcc = 2.7–3.6 V):
     *   0–30 MHz  → 0 WS
     *   31–60 MHz → 1 WS
     *   61–90 MHz → 2 WS
     *   91–120 MHz→ 3 WS
     *  121–150 MHz→ 4 WS
     *  151–168 MHz→ 5 WS  ← our case
     *
     * HAL_RCC_ClockConfig() automatically sets the Flash wait-states
     * when passed FLASH_LATENCY_5.
     */
    RCC_ClkInitTypeDef clk_cfg = {0};

    clk_cfg.ClockType = RCC_CLOCKTYPE_HCLK  |
                        RCC_CLOCKTYPE_SYSCLK |
                        RCC_CLOCKTYPE_PCLK1  |
                        RCC_CLOCKTYPE_PCLK2;

    clk_cfg.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;  /* use PLL output   */
    clk_cfg.AHBCLKDivider  = RCC_SYSCLK_DIV1;          /* AHB  = 168 MHz   */
    clk_cfg.APB1CLKDivider = RCC_HCLK_DIV4;            /* APB1 =  42 MHz   */
    clk_cfg.APB2CLKDivider = RCC_HCLK_DIV2;            /* APB2 =  84 MHz   */

    if (HAL_RCC_ClockConfig(&clk_cfg, FLASH_LATENCY_5) != HAL_OK)
    {
        Error_Handler();
    }

    /* ── 5. Enable Flash instruction cache, data cache, prefetch ──
     *
     * These improve performance from Flash at high clock speeds.
     * Instruction cache: avoids repeated Flash reads for loops.
     * Data cache:        caches Flash constant/table data.
     * Prefetch:          pre-loads next Flash line while CPU executes.
     */
    __HAL_FLASH_INSTRUCTION_CACHE_ENABLE();
    __HAL_FLASH_DATA_CACHE_ENABLE();
    __HAL_FLASH_PREFETCH_BUFFER_ENABLE();

    /* ── 6. Update SystemCoreClock variable ───────────────────────
     *
     * HAL_RCC_ClockConfig() updates SystemCoreClock internally,
     * but calling SystemCoreClockUpdate() ensures CMSIS and any
     * third-party code that reads SystemCoreClock is consistent.
     */
    SystemCoreClockUpdate();
}

/* ------------------------------------------------------------------ */

uint32_t SystemClock_GetFreqHz(void)
{
    return HAL_RCC_GetSysClockFreq();
}
