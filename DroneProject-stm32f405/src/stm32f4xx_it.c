/**
 * @file    stm32f4xx_it.c
 * @brief   STM32F4 interrupt service routines
 *
 * Wire peripheral ISR wrappers here.  Each driver exposes a named handler
 * (e.g. SBUS_USART_IRQHandler) so the vector entry stays thin.
 *
 * How to add a new driver ISR:
 *   1. Add the driver's handler declaration in its .h file.
 *   2. Add a one-line call here inside the matching IRQ function.
 */

#include "stm32f4xx_hal.h"
#include "drivers/sbus.h"
#include "drivers/gps.h"

/* ── SysTick ─────────────────────────────────────────────────────────
 * HAL_IncTick() must be called every 1 ms for HAL_Delay / HAL_GetTick.
 * PlatformIO / STM32Cube auto-generates this, but defining it here is
 * safe — the linker will use this definition and discard any weak one.
 * ------------------------------------------------------------------ */
void SysTick_Handler(void)
{
    HAL_IncTick();
}

/* ── USART1 — SBUS IDLE-line detection ───────────────────────────── */
void USART1_IRQHandler(void)
{
    SBUS_USART_IRQHandler();
}

/* ── DMA2 Stream5 — SBUS circular RX ────────────────────────────── */
void DMA2_Stream5_IRQHandler(void)
{
    SBUS_DMA_IRQHandler();
}

/* ── USART6 — GPS IDLE-line detection ────────────────────────────── */
void USART6_IRQHandler(void)
{
    GPS_USART_IRQHandler();
}

/* ── DMA2 Stream1 — GPS circular RX ─────────────────────────────── */
void DMA2_Stream1_IRQHandler(void)
{
    GPS_DMA_IRQHandler();
}

/* ── USB OTG FS ──────────────────────────────────────────────────── */
void OTG_FS_IRQHandler(void)
{
    extern PCD_HandleTypeDef hpcd_USB_OTG_FS;
    HAL_PCD_IRQHandler(&hpcd_USB_OTG_FS);
}
