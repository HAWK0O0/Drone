/**
 * @file    telemetry.c
 * @brief   MAVLink v1 telemetry over USART2 (PA2 TX, PA3 RX) to ESP32-S3CAM
 *
 * ── USART2 config ─────────────────────────────────────────────────────
 *   PA2 = TX (AF7)    PA3 = RX (AF7)
 *   115200 baud, 8N1, blocking TX (HAL_UART_Transmit, 10 ms timeout)
 *
 * ── Messages sent (call Telemetry_Send() at 10 Hz) ────────────────────
 *   HEARTBEAT   (0)  — system type, arm state
 *   SYS_STATUS  (1)  — CPU load = 0 (no RTOS), battery unknown
 *   GPS_RAW_INT (24) — lat/lon/alt/sats
 *   ATTITUDE    (30) — roll/pitch/yaw [rad], gyro rates from IMU [rad/s]
 *   RC_CHANNELS (65) — 16 SBUS channels mapped to µs
 *
 * ── RX (ESP32 → FC) ───────────────────────────────────────────────────
 *   Stub — no commands parsed yet.  DMA RX can be wired later.
 */

#include "telemetry/telemetry.h"
#include "telemetry/mavlink_minimal.h"
#include "ahrs/ahrs.h"
#include "drivers/imu.h"
#include "drivers/gps.h"
#include "drivers/sbus.h"
#include "safety.h"
#include "pin_config.h"
#include <string.h>

/* ── Private state ────────────────────────────────────────────────── */
static UART_HandleTypeDef s_huart;
static uint8_t            s_seq = 0;    /* MAVLink sequence counter     */
static uint8_t            s_ready = 0;

#define TEL_TX_TIMEOUT_MS  10U          /* per-message blocking timeout  */

/* ── Private helper ───────────────────────────────────────────────── */

static void send_frame(uint8_t msg_id, uint8_t crc_extra,
                       const void *payload, uint8_t len)
{
    if (!s_ready) return;
    uint8_t frame[128];
    uint8_t frame_len = mavlink_pack(frame, msg_id, crc_extra, payload, len, &s_seq);
    HAL_UART_Transmit(&s_huart, frame, frame_len, TEL_TX_TIMEOUT_MS);
}

/* ── Public API ────────────────────────────────────────────────────── */

void Telemetry_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};
    g.Pin       = CAM_TX_PIN | CAM_RX_PIN;   /* PA2, PA3 */
    g.Mode      = GPIO_MODE_AF_PP;
    g.Pull      = GPIO_NOPULL;
    g.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(CAM_TX_PORT, &g);

    s_huart.Instance          = USART2;
    s_huart.Init.BaudRate     = 115200U;
    s_huart.Init.WordLength   = UART_WORDLENGTH_8B;
    s_huart.Init.StopBits     = UART_STOPBITS_1;
    s_huart.Init.Parity       = UART_PARITY_NONE;
    s_huart.Init.Mode         = UART_MODE_TX_RX;
    s_huart.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    s_huart.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&s_huart) != HAL_OK) return;
    s_ready = 1;
}

void Telemetry_Send(void)
{
    Telemetry_SendHeartbeat();
    Telemetry_SendSysStatus();
    Telemetry_SendGPS();
    Telemetry_SendAttitude();
    Telemetry_SendRC();
}

void Telemetry_Receive(void)
{
    /* Stub — MAVLink command parsing (PARAM_SET, COMMAND_LONG) reserved */
}

void Telemetry_SendHeartbeat(void)
{
    arm_state_t state = Safety_GetState();

    uint8_t base_mode = MAV_MODE_FLAG_CUSTOM_MODE_EN;
    uint8_t sys_status;

    switch (state)
    {
        case ARM_STATE_ARMED:
            base_mode |= MAV_MODE_FLAG_SAFETY_ARMED | MAV_MODE_FLAG_MANUAL_INPUT_EN;
            sys_status = MAV_STATE_ACTIVE;
            break;
        case ARM_STATE_FAILSAFE:
            sys_status = MAV_STATE_CRITICAL;
            break;
        case ARM_STATE_ERROR:
            sys_status = MAV_STATE_EMERGENCY;
            break;
        default:
            sys_status = MAV_STATE_STANDBY;
            break;
    }

    mav_heartbeat_t hb = {
        .custom_mode     = 0U,
        .type            = MAV_TYPE_QUADROTOR,
        .autopilot       = MAV_AUTOPILOT_GENERIC,
        .base_mode       = base_mode,
        .system_status   = sys_status,
        .mavlink_version = 3U,
    };

    send_frame(MAVLINK_MSG_ID_HEARTBEAT, MAVLINK_CRC_EXTRA_HEARTBEAT,
               &hb, (uint8_t)sizeof(hb));
}

void Telemetry_SendSysStatus(void)
{
    /* Sensors bitmask: 3D gyro (0x1) + 3D accel (0x2) + GPS (0x20) */
    const uint32_t sns = 0x00000023UL;

    mav_sys_status_t s = {
        .sensors_present    = sns,
        .sensors_enabled    = sns,
        .sensors_health     = sns,
        .load               = 0U,       /* CPU load unknown (no RTOS) */
        .voltage_battery    = 0U,       /* no ADC yet — 0 = unknown   */
        .current_battery    = -1,
        .drop_rate_comm     = 0U,
        .errors_comm        = 0U,
        .errors_count1      = 0U,
        .errors_count2      = 0U,
        .errors_count3      = 0U,
        .errors_count4      = 0U,
        .battery_remaining  = -1,
    };

    send_frame(MAVLINK_MSG_ID_SYS_STATUS, MAVLINK_CRC_EXTRA_SYS_STATUS,
               &s, (uint8_t)sizeof(s));
}

void Telemetry_SendGPS(void)
{
    const gps_data_t *gps = GPS_GetData();

    uint8_t fix = GPS_FIX_TYPE_NO_FIX;
    if (gps->fix >= 1U) fix = GPS_FIX_TYPE_3D_FIX;

    mav_gps_raw_int_t g = {
        .time_usec          = (uint64_t)HAL_GetTick() * 1000ULL,
        .lat                = (int32_t)(gps->latitude  * 1e7),
        .lon                = (int32_t)(gps->longitude * 1e7),
        .alt                = (int32_t)(gps->altitude_m * 1000.0f),
        .eph                = (gps->hdop > 0.0f) ?
                                  (uint16_t)(gps->hdop * 100.0f) : 0xFFFFU,
        .epv                = 0xFFFFU,
        .vel                = (uint16_t)(gps->speed_mps * 100.0f),
        .cog                = 0xFFFFU,
        .fix_type           = fix,
        .satellites_visible = gps->satellites,
    };

    send_frame(MAVLINK_MSG_ID_GPS_RAW_INT, MAVLINK_CRC_EXTRA_GPS_RAW_INT,
               &g, (uint8_t)sizeof(g));
}

void Telemetry_SendAttitude(void)
{
    const ahrs_state_t *att = AHRS_GetState();

    /* Gyro rates: not stored in ahrs_state_t — read fresh from IMU */
    imu_data_t imu = {0};
    (void)IMU_Read(&imu);

    static const float DEG2RAD = (float)(3.14159265358979323846 / 180.0);

    mav_attitude_t a = {
        .time_boot_ms = HAL_GetTick(),
        .roll         = att->roll  * DEG2RAD,
        .pitch        = att->pitch * DEG2RAD,
        .yaw          = att->yaw   * DEG2RAD,
        .rollspeed    = imu.gyro_x * DEG2RAD,
        .pitchspeed   = imu.gyro_y * DEG2RAD,
        .yawspeed     = imu.gyro_z * DEG2RAD,
    };

    send_frame(MAVLINK_MSG_ID_ATTITUDE, MAVLINK_CRC_EXTRA_ATTITUDE,
               &a, (uint8_t)sizeof(a));
}

void Telemetry_SendRC(void)
{
    const sbus_data_t *rc = SBUS_GetData();

    mav_rc_channels_t r;
    r.time_boot_ms = HAL_GetTick();
    r.chan1_raw  = SBUS_ToMicros(rc->channel[0]);
    r.chan2_raw  = SBUS_ToMicros(rc->channel[1]);
    r.chan3_raw  = SBUS_ToMicros(rc->channel[2]);
    r.chan4_raw  = SBUS_ToMicros(rc->channel[3]);
    r.chan5_raw  = SBUS_ToMicros(rc->channel[4]);
    r.chan6_raw  = SBUS_ToMicros(rc->channel[5]);
    r.chan7_raw  = SBUS_ToMicros(rc->channel[6]);
    r.chan8_raw  = SBUS_ToMicros(rc->channel[7]);
    r.chan9_raw  = SBUS_ToMicros(rc->channel[8]);
    r.chan10_raw = SBUS_ToMicros(rc->channel[9]);
    r.chan11_raw = SBUS_ToMicros(rc->channel[10]);
    r.chan12_raw = SBUS_ToMicros(rc->channel[11]);
    r.chan13_raw = SBUS_ToMicros(rc->channel[12]);
    r.chan14_raw = SBUS_ToMicros(rc->channel[13]);
    r.chan15_raw = SBUS_ToMicros(rc->channel[14]);
    r.chan16_raw = SBUS_ToMicros(rc->channel[15]);
    r.chan17_raw = 0U;
    r.chan18_raw = 0U;
    r.chancount  = 16U;
    r.rssi       = rc->failsafe ? 0U : 255U;

    send_frame(MAVLINK_MSG_ID_RC_CHANNELS, MAVLINK_CRC_EXTRA_RC_CHANNELS,
               &r, (uint8_t)sizeof(r));
}
