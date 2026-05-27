/**
 * @file    blackbox.c
 * @brief   Flight data recorder — buffered CSV log to SD card via FatFS
 *
 * ── Lifecycle ─────────────────────────────────────────────────────────
 *   Blackbox_Init()   — reset state only; no file opened
 *   Blackbox_Open()   — called on ARM: find next free LOG_NNN.CSV, open it
 *   Blackbox_Log()    — called at 50 Hz (every 20 ms) while armed
 *   Blackbox_Close()  — called on DISARM: flush + close; no data is lost
 *
 * ── Log format (CSV) ──────────────────────────────────────────────────
 *   time_ms, roll, pitch, yaw,
 *   gx, gy, gz  [deg/s],
 *   ax, ay, az  [m/s²],
 *   thr_us, roll_us, pit_us, yaw_us,
 *   arm
 *
 * ── Write strategy ────────────────────────────────────────────────────
 *   Rows are accumulated in a 2 kB text buffer; flushed to SD whenever
 *   the buffer is ≥ half full.  f_sync() is called every BB_SYNC_ROWS
 *   (= 50 rows ≈ 1 s at 50 Hz) to commit data to the card.
 */

#include "blackbox.h"
#include "drivers/sdcard.h"
#include "drivers/imu.h"
#include "ahrs/ahrs.h"
#include "drivers/sbus.h"
#include "safety.h"
#include <stdio.h>
#include <string.h>

/* ── Configuration ────────────────────────────────────────────────── */
#define BB_BUF_SIZE     2048U   /* write-behind text buffer (bytes)  */
#define BB_FLUSH_LVL    1024U   /* flush when buffer exceeds this    */
#define BB_SYNC_ROWS    50U     /* f_sync() after this many rows     */

static const char BB_HEADER[] =
    "time_ms,roll,pitch,yaw,gx,gy,gz,ax,ay,az,"
    "thr_us,roll_us,pit_us,yaw_us,arm\r\n";

/* ── Module state ─────────────────────────────────────────────────── */
static uint8_t  s_active    = 0;
static char     s_buf[BB_BUF_SIZE];
static uint16_t s_buf_len   = 0;
static uint32_t s_row_count = 0;

/* ── Private helpers ──────────────────────────────────────────────── */

static void flush_buffer(void)
{
    if (s_buf_len == 0) return;
    SDCard_Write(s_buf, s_buf_len);
    s_buf_len = 0;
}

/* ── Public API ────────────────────────────────────────────────────── */

void Blackbox_Init(void)
{
    s_active    = 0;
    s_buf_len   = 0;
    s_row_count = 0;
}

void Blackbox_Open(void)
{
    if (s_active) return;           /* already open — shouldn't happen */

    s_buf_len   = 0;
    s_row_count = 0;

    char fname[16];
    sdcard_status_t st = SDCARD_WRITE_ERR;

    for (int n = 1; n <= 999; n++)
    {
        (void)snprintf(fname, sizeof(fname), "LOG_%03d.CSV", n);
        st = SDCard_OpenLog(fname);  /* FA_CREATE_NEW: fails if exists */
        if (st == SDCARD_OK) break;
    }

    if (st != SDCARD_OK) return;     /* no card or all 999 names used  */

    SDCard_Write(BB_HEADER, (uint32_t)(sizeof(BB_HEADER) - 1U));
    SDCard_Sync();
    s_active = 1;
}

void Blackbox_Log(void)
{
    if (!s_active) return;

    const ahrs_state_t *att = AHRS_GetState();
    const sbus_data_t  *rc  = SBUS_GetData();

    imu_data_t imu = {0};
    (void)IMU_Read(&imu);

    uint16_t thr_us  = SBUS_ToMicros(rc->channel[SBUS_CH_THROTTLE]);
    uint16_t roll_us = SBUS_ToMicros(rc->channel[SBUS_CH_AILERON]);
    uint16_t pit_us  = SBUS_ToMicros(rc->channel[SBUS_CH_ELEVATOR]);
    uint16_t yaw_us  = SBUS_ToMicros(rc->channel[SBUS_CH_RUDDER]);

    char row[128];
    int  n = snprintf(row, sizeof(row),
        "%lu,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%u,%u,%u,%u,1\r\n",
        (unsigned long)HAL_GetTick(),
        (double)att->roll,   (double)att->pitch,   (double)att->yaw,
        (double)imu.gyro_x,  (double)imu.gyro_y,   (double)imu.gyro_z,
        (double)imu.accel_x, (double)imu.accel_y,  (double)imu.accel_z,
        (unsigned)thr_us, (unsigned)roll_us,
        (unsigned)pit_us, (unsigned)yaw_us);

    if (n <= 0 || (uint16_t)n >= sizeof(row)) return;

    if (s_buf_len + (uint16_t)n >= BB_BUF_SIZE)
        flush_buffer();

    memcpy(s_buf + s_buf_len, row, (size_t)n);
    s_buf_len += (uint16_t)n;
    s_row_count++;

    if (s_buf_len >= BB_FLUSH_LVL) flush_buffer();

    if (s_row_count % BB_SYNC_ROWS == 0)
    {
        flush_buffer();
        SDCard_Sync();
    }
}

void Blackbox_Close(void)
{
    if (!s_active) return;
    flush_buffer();
    SDCard_Sync();
    SDCard_CloseLog();
    s_active    = 0;
    s_buf_len   = 0;
    s_row_count = 0;
}

uint8_t Blackbox_IsActive(void)
{
    return s_active;
}
