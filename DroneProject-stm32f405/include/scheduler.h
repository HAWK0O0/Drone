/**
 * @file    scheduler.h
 * @brief   Bare-metal fixed-rate task scheduler using DWT cycle counter
 */

#ifndef FC_SCHEDULER_H
#define FC_SCHEDULER_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

/** Initialise DWT cycle counter for high-resolution timing */
void Scheduler_Init(void);

/** Block until the next 1 kHz tick. Call at the top of the main loop. */
void Scheduler_WaitNextTick(void);

/** Returns time since boot in milliseconds */
uint32_t Scheduler_GetTickMs(void);

/** Returns time since boot in microseconds (uses DWT) */
uint32_t Scheduler_GetTickUs(void);

/** Returns 1 if a sub-rate task should run this cycle.
 *  @param rate_hz  Desired task rate (must be a divisor of MAIN_LOOP_RATE_HZ)
 */
uint8_t Scheduler_ShouldRunTask(uint32_t rate_hz);

#endif /* FC_SCHEDULER_H */
