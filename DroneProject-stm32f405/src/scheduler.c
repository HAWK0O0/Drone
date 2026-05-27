/**
 * @file    scheduler.c
 * @brief   Bare-metal fixed-rate task scheduler — DWT cycle counter
 *
 * Uses the ARM Cortex-M4 DWT (Data Watchpoint and Trace) CYCCNT register
 * to produce a hardware-accurate 1 kHz tick with sub-microsecond jitter.
 *
 * Cycle maths at 168 MHz:
 *   CYCLES_PER_TICK = 168,000,000 / 1000 = 168,000
 *   1 DWT count     = 1 / 168,000,000 s ≈ 5.95 ns
 *
 * Sub-rate task scheduling:
 *   ShouldRunTask(rate_hz) returns 1 when tick_count % (1000 / rate_hz) == 0
 *   i.e. every N-th 1 ms tick, where N = MAIN_LOOP_RATE_HZ / rate_hz.
 *   rate_hz must be a divisor of MAIN_LOOP_RATE_HZ.
 */

#include "scheduler.h"
#include "config.h"

/* DWT register access (CMSIS CoreDebug / DWT not always exposed in older HAL) */
#ifndef DWT_CTRL_CYCCNTENA_Msk
#define DWT_CTRL_CYCCNTENA_Msk  (1UL << 0)
#endif
#ifndef CoreDebug_DEMCR_TRCENA_Msk
#define CoreDebug_DEMCR_TRCENA_Msk (1UL << 24)
#endif

/* Cycles per 1 ms tick at 168 MHz */
#define CYCLES_PER_TICK  (SystemCoreClock / MAIN_LOOP_RATE_HZ)

/* Module state */
static uint32_t s_last_cyc = 0U;   /* DWT cycle count at last tick start */
static uint32_t s_tick     = 0U;   /* Incremented every WaitNextTick()   */

/* ------------------------------------------------------------------ */

void Scheduler_Init(void)
{
    /* Enable DWT trace unit */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;

    s_last_cyc = DWT->CYCCNT;
    s_tick     = 0U;
}

/* ------------------------------------------------------------------ */

void Scheduler_WaitNextTick(void)
{
    /* Advance target by exactly one tick period */
    uint32_t target = s_last_cyc + CYCLES_PER_TICK;

    /* Spin-wait using signed subtraction to handle 32-bit CYCCNT wrap */
    while ((int32_t)(DWT->CYCCNT - target) < 0) {}

    s_last_cyc = target;
    s_tick++;
}

/* ------------------------------------------------------------------ */

uint32_t Scheduler_GetTickMs(void)
{
    return HAL_GetTick();
}

/* ------------------------------------------------------------------ */

uint32_t Scheduler_GetTickUs(void)
{
    /* DWT ticks / (168 ticks/µs) */
    return DWT->CYCCNT / (SystemCoreClock / 1000000U);
}

/* ------------------------------------------------------------------ */

uint8_t Scheduler_ShouldRunTask(uint32_t rate_hz)
{
    if (rate_hz == 0U || rate_hz > MAIN_LOOP_RATE_HZ) { return 0U; }

    uint32_t period = MAIN_LOOP_RATE_HZ / rate_hz;   /* e.g. 1000/100 = 10 */
    return (s_tick % period == 0U) ? 1U : 0U;
}
