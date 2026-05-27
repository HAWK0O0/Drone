/**
 * @file    blackbox.h
 * @brief   Flight data recorder — buffered CSV log to SD card
 *
 * Usage:
 *   1. Call Blackbox_Init() once after SDCard_Init() at boot.
 *   2. Call Blackbox_Open()  when the drone arms  → creates a new log file.
 *   3. Call Blackbox_Log()   at 50 Hz (every 20 ms) while armed.
 *   4. Call Blackbox_Close() when the drone disarms → flushes & closes file.
 */

#ifndef FC_BLACKBOX_H
#define FC_BLACKBOX_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

/** Initialise module state (SD must be mounted first). Does NOT open a file. */
void Blackbox_Init(void);

/** Open a new auto-numbered log file (LOG_001.CSV … LOG_999.CSV). */
void Blackbox_Open(void);

/** Write one log entry — call at 50 Hz (every 20 ms) while armed. */
void Blackbox_Log(void);

/** Flush buffer to SD and close the log file. */
void Blackbox_Close(void);

/** Returns 1 if a log file is currently open and receiving data. */
uint8_t Blackbox_IsActive(void);

#endif /* FC_BLACKBOX_H */
