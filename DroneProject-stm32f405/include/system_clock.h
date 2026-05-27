/**
 * @file    system_clock.h
 * @brief   System clock configuration for STM32F405RGT6
 *
 * Clock tree summary:
 * ┌─────────────────────────────────────────────────────────┐
 * │  HSE (8 MHz)  ──► PLL_M÷8 ──► 1 MHz                   │
 * │                   PLL_N×336 ──► 336 MHz (VCO)          │
 * │                   PLL_P÷2  ──► 168 MHz  (SYSCLK)       │
 * │                   PLL_Q÷7  ──► 48 MHz   (USB OTG)      │
 * │                                                         │
 * │  SYSCLK  168 MHz ──► AHB  ÷1  ──► HCLK   = 168 MHz    │
 * │                       APB1 ÷4  ──► PCLK1  =  42 MHz    │
 * │                       APB2 ÷2  ──► PCLK2  =  84 MHz    │
 * │                                                         │
 * │  Flash: 5 wait-states @ 168 MHz / 3.3 V                │
 * │  FPU:   CP10 + CP11 full access (Cortex-M4 FPU)        │
 * └─────────────────────────────────────────────────────────┘
 *
 * Peripheral clocks derived:
 *   TIM1, TIM8–TIM11 (APB2 timers) : 168 MHz
 *   TIM2–TIM7, TIM12–TIM14 (APB1)  :  84 MHz
 *   I2C1/2, USART2/3 (APB1 peri.)  :  42 MHz
 *   SPI1, USART1/6 (APB2 peri.)    :  84 MHz
 */

#ifndef FC_SYSTEM_CLOCK_H
#define FC_SYSTEM_CLOCK_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

/* PLL coefficients — change only if HSE frequency differs */
#define CLOCK_PLL_M     8U    /**< HSE ÷ M  = 1 MHz VCO input      */
#define CLOCK_PLL_N   336U    /**< 1 MHz × N = 336 MHz VCO output   */
#define CLOCK_PLL_P     2U    /**< 336 ÷ 2  = 168 MHz SYSCLK        */
#define CLOCK_PLL_Q     7U    /**< 336 ÷ 7  ≈ 48 MHz USB            */

#define CLOCK_HSE_MHZ   8U    /**< WeAct board crystal frequency     */
#define CLOCK_SYS_MHZ 168U    /**< Target system clock               */

/**
 * @brief  Enable Cortex-M4 FPU and configure all clocks to 168 MHz.
 *
 * Must be the FIRST call after HAL_Init() in main().
 * Execution blocks until PLL is locked (hardware guaranteed < 2 ms).
 *
 * On HAL error the function calls Error_Handler() — implement it in main.c.
 */
void SystemClock_Config(void);

/**
 * @brief  Returns the measured SYSCLK in Hz (via HAL_RCC_GetSysClockFreq).
 *         Useful for a runtime sanity-check after init.
 */
uint32_t SystemClock_GetFreqHz(void);

/**
 * @brief  Called by HAL on RCC / clock errors. Infinite loop with LED toggle.
 *         Weak implementation here — override in main.c for custom handling.
 */
void Error_Handler(void);

#endif /* FC_SYSTEM_CLOCK_H */
