/**
 * @file    sdcard.h
 * @brief   SD card driver via SDIO (PC8–PC12, PD2) + FatFS
 *          Card detect on PA8
 */

#ifndef FC_SDCARD_H
#define FC_SDCARD_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

typedef enum {
    SDCARD_OK       = 0,
    SDCARD_NO_CARD,
    SDCARD_MOUNT_ERR,
    SDCARD_WRITE_ERR,
    SDCARD_NOT_INIT
} sdcard_status_t;

/** Initialise SDIO peripheral and mount FatFS volume */
sdcard_status_t SDCard_Init(void);

/** Returns 1 if a card is physically inserted (PA8 detect pin) */
uint8_t SDCard_IsInserted(void);

/** Open or create a log file. filename = "LOG_XXX.CSV" */
sdcard_status_t SDCard_OpenLog(const char *filename);

/** Write a null-terminated string to the open log file */
sdcard_status_t SDCard_Write(const char *data, uint32_t len);

/** Flush write cache to card */
sdcard_status_t SDCard_Sync(void);

/** Close the open log file */
void SDCard_CloseLog(void);

/** Unmount FatFS and deinit SDIO */
void SDCard_Deinit(void);

#endif /* FC_SDCARD_H */
