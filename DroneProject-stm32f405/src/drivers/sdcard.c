/**
 * @file    sdcard.c
 * @brief   SD card driver via SDIO 4-bit mode + FatFS
 *
 * ── Pins ──────────────────────────────────────────────────────────────
 *   PC8  = D0  (AF12, pull-up)       PC9  = D1  (AF12, pull-up)
 *   PC10 = D2  (AF12, pull-up)       PC11 = D3  (AF12, pull-up)
 *   PC12 = CLK (AF12, no pull)       PD2  = CMD (AF12, pull-up)
 *   PA8  = card detect (input, pull-up, active LOW)
 *
 * ── Clock ─────────────────────────────────────────────────────────────
 *   SDIOCLK = PLLQ = 48 MHz
 *   CLKDIV  = 0  →  SDCLK = 48 / (0+2) = 24 MHz  (SD spec max 25 MHz)
 *
 * ── FatFS ────────────────────────────────────────────────────────────
 *   Single volume (drive ""), mounted at SDCard_Init().
 *   Disk I/O is provided by src/drivers/sd_diskio.c.
 */

#include "drivers/sdcard.h"
#include "pin_config.h"
#include "ff.h"         /* FatFS public API           */
#include "diskio.h"     /* DSTATUS / DRESULT types    */
#include <string.h>

/* ── Module state ─────────────────────────────────────────────────── */
SD_HandleTypeDef hsd;          /* used by sd_diskio.c via extern   */
static FATFS     s_fs;         /* FatFS workspace                   */
static FIL       s_file;       /* currently open log file           */
static uint8_t   s_mounted  = 0;
static uint8_t   s_file_open = 0;

/* ── HAL MSP — called internally by HAL_SD_Init() ─────────────────── */
void HAL_SD_MspInit(SD_HandleTypeDef *sd)
{
    (void)sd;

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_SDIO_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};

    /* D0–D3 on PC8–PC11 (AF12, pull-up) */
    g.Pin       = SDIO_D0_PIN | SDIO_D1_PIN | SDIO_D2_PIN | SDIO_D3_PIN;
    g.Mode      = GPIO_MODE_AF_PP;
    g.Pull      = GPIO_PULLUP;
    g.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = GPIO_AF12_SDIO;
    HAL_GPIO_Init(SDIO_PORT, &g);

    /* CLK on PC12 (AF12, no pull — spec requirement) */
    g.Pin       = SDIO_CLK_PIN;
    g.Pull      = GPIO_NOPULL;
    HAL_GPIO_Init(SDIO_PORT, &g);

    /* CMD on PD2 (AF12, pull-up) */
    g.Pin       = SDIO_CMD_PIN;
    g.Pull      = GPIO_PULLUP;
    HAL_GPIO_Init(SDIO_CMD_PORT, &g);

    /* Card detect on PA8 (input, pull-up, active LOW) */
    g.Pin       = SDIO_DETECT_PIN;
    g.Mode      = GPIO_MODE_INPUT;
    g.Pull      = GPIO_PULLUP;
    g.Alternate = 0;
    HAL_GPIO_Init(SDIO_DETECT_PORT, &g);
}

void HAL_SD_MspDeInit(SD_HandleTypeDef *sd)
{
    (void)sd;
    HAL_GPIO_DeInit(SDIO_PORT,      SDIO_D0_PIN | SDIO_D1_PIN |
                                    SDIO_D2_PIN | SDIO_D3_PIN | SDIO_CLK_PIN);
    HAL_GPIO_DeInit(SDIO_CMD_PORT,  SDIO_CMD_PIN);
    HAL_GPIO_DeInit(SDIO_DETECT_PORT, SDIO_DETECT_PIN);
    __HAL_RCC_SDIO_CLK_DISABLE();
}

/* ── Public API ────────────────────────────────────────────────────── */

uint8_t SDCard_IsInserted(void)
{
    /* Card detect is active LOW */
    return (HAL_GPIO_ReadPin(SDIO_DETECT_PORT, SDIO_DETECT_PIN) == GPIO_PIN_RESET) ? 1U : 0U;
}

sdcard_status_t SDCard_Init(void)
{
    if (!SDCard_IsInserted()) return SDCARD_NO_CARD;

    /* Configure SDIO peripheral */
    hsd.Instance                 = SDIO;
    hsd.Init.ClockEdge           = SDIO_CLOCK_EDGE_RISING;
    hsd.Init.ClockBypass         = SDIO_CLOCK_BYPASS_DISABLE;
    hsd.Init.ClockPowerSave      = SDIO_CLOCK_POWER_SAVE_DISABLE;
    hsd.Init.BusWide             = SDIO_BUS_WIDE_1B;   /* 1-bit for init sequence */
    hsd.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
    hsd.Init.ClockDiv            = 0U;                 /* SDCLK = 48/2 = 24 MHz   */

    if (HAL_SD_Init(&hsd) != HAL_OK)                   return SDCARD_MOUNT_ERR;

    /* Switch to 4-bit bus after successful init */
    if (HAL_SD_ConfigWideBusOperation(&hsd, SDIO_BUS_WIDE_4B) != HAL_OK)
        return SDCARD_MOUNT_ERR;

    /* Mount FatFS volume (opt=1: mount immediately → calls disk_initialize) */
    FRESULT fr = f_mount(&s_fs, "", 1);
    if (fr != FR_OK) return SDCARD_MOUNT_ERR;

    s_mounted = 1;
    return SDCARD_OK;
}

sdcard_status_t SDCard_OpenLog(const char *filename)
{
    if (!s_mounted)    return SDCARD_NOT_INIT;
    if (s_file_open)   { f_close(&s_file); s_file_open = 0; }

    /* FA_CREATE_NEW returns FR_EXIST if the file already exists.
     * Blackbox_Init() uses this to auto-increment LOG_001 → LOG_999. */
    FRESULT fr = f_open(&s_file, filename, FA_WRITE | FA_CREATE_NEW);
    if (fr != FR_OK) return SDCARD_WRITE_ERR;

    s_file_open = 1;
    return SDCARD_OK;
}

sdcard_status_t SDCard_Write(const char *data, uint32_t len)
{
    if (!s_mounted || !s_file_open) return SDCARD_NOT_INIT;

    UINT bw;
    FRESULT fr = f_write(&s_file, data, (UINT)len, &bw);
    if (fr != FR_OK || bw != (UINT)len) return SDCARD_WRITE_ERR;

    return SDCARD_OK;
}

sdcard_status_t SDCard_Sync(void)
{
    if (!s_mounted || !s_file_open) return SDCARD_NOT_INIT;
    return (f_sync(&s_file) == FR_OK) ? SDCARD_OK : SDCARD_WRITE_ERR;
}

void SDCard_CloseLog(void)
{
    if (s_file_open)
    {
        f_sync(&s_file);
        f_close(&s_file);
        s_file_open = 0;
    }
}

void SDCard_Deinit(void)
{
    SDCard_CloseLog();
    f_mount(NULL, "", 0);
    HAL_SD_DeInit(&hsd);
    s_mounted = 0;
}
