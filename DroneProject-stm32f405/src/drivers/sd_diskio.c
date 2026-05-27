/**
 * @file    sd_diskio.c
 * @brief   FatFS low-level disk I/O — bridges disk_* callbacks to HAL_SD (SDIO).
 *          Compiled as part of the project (not a middleware template).
 *
 * Physical drive number 0 = SD card via SDIO.
 *
 * @note  hsd is defined in sdcard.c and externally referenced here.
 *        Call SDCard_Init() before mounting the FatFS volume.
 */

#include "diskio.h"      /* FatFS disk I/O interface  */
#include "stm32f4xx_hal.h"
#include <string.h>

/* Sector size — fixed 512 bytes; matches FF_MIN_SS = FF_MAX_SS = 512 */
#define SECTOR_SIZE  512U

/* HAL timeout for blocking read/write operations (ms) */
#define DISK_TIMEOUT_MS  5000U

/* SD handle is owned by sdcard.c */
extern SD_HandleTypeDef hsd;

/* -------------------------------------------------------------------------- */

DSTATUS disk_initialize(BYTE pdrv)
{
    if (pdrv != 0) return STA_NOINIT;

    /* hsd was already fully initialised (4-bit bus) by SDCard_Init().
     * Just confirm the card is in TRANSFER state before allowing I/O. */
    if (HAL_SD_GetCardState(&hsd) != HAL_SD_CARD_TRANSFER) return STA_NOINIT;

    return 0; /* RDY */
}

DSTATUS disk_status(BYTE pdrv)
{
    if (pdrv != 0) return STA_NOINIT;
    if (HAL_SD_GetCardState(&hsd) != HAL_SD_CARD_TRANSFER) return STA_NOINIT;
    return 0;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count)
{
    if (pdrv != 0 || count == 0) return RES_PARERR;

    if (HAL_SD_ReadBlocks(&hsd, buff, (uint32_t)sector, (uint32_t)count,
                          DISK_TIMEOUT_MS) != HAL_OK)
        return RES_ERROR;

    /* Wait until card returns to TRANSFER state after multi-block read */
    uint32_t tick = HAL_GetTick();
    while (HAL_SD_GetCardState(&hsd) != HAL_SD_CARD_TRANSFER)
    {
        if (HAL_GetTick() - tick > DISK_TIMEOUT_MS) return RES_ERROR;
    }
    return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count)
{
    if (pdrv != 0 || count == 0) return RES_PARERR;

    /* HAL_SD_WriteBlocks takes non-const; cast is safe — we do not modify data */
    if (HAL_SD_WriteBlocks(&hsd, (uint8_t *)buff, (uint32_t)sector,
                           (uint32_t)count, DISK_TIMEOUT_MS) != HAL_OK)
        return RES_ERROR;

    /* Wait until card finishes programming flash and returns to TRANSFER state */
    uint32_t tick = HAL_GetTick();
    while (HAL_SD_GetCardState(&hsd) != HAL_SD_CARD_TRANSFER)
    {
        if (HAL_GetTick() - tick > DISK_TIMEOUT_MS) return RES_ERROR;
    }
    return RES_OK;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    if (pdrv != 0) return RES_PARERR;

    switch (cmd)
    {
        case CTRL_SYNC:
            /* Nothing to flush — writes are committed synchronously above */
            return RES_OK;

        case GET_SECTOR_COUNT:
        {
            HAL_SD_CardInfoTypeDef info;
            if (HAL_SD_GetCardInfo(&hsd, &info) != HAL_OK) return RES_ERROR;
            *(DWORD *)buff = info.LogBlockNbr;
            return RES_OK;
        }

        case GET_SECTOR_SIZE:
            /* Fixed 512 bytes — only needed when FF_MAX_SS != FF_MIN_SS */
            *(WORD *)buff = (WORD)SECTOR_SIZE;
            return RES_OK;

        case GET_BLOCK_SIZE:
            /* Erase block size in sectors — 1 = unknown / default */
            *(DWORD *)buff = 1U;
            return RES_OK;

        default:
            return RES_PARERR;
    }
}
