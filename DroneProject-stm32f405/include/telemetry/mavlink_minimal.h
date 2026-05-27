/**
 * @file    mavlink_minimal.h
 * @brief   Self-contained MAVLink v1 frame builder — no external library needed.
 *
 * Implements:
 *   - CRC-16/IBM-SDLC (X25) without a lookup table
 *   - mavlink_pack()  — assemble a complete frame into caller's buffer
 *   - Packed payload structs for 5 messages:
 *       HEARTBEAT (0), SYS_STATUS (1), GPS_RAW_INT (24), ATTITUDE (30),
 *       RC_CHANNELS (65)
 *
 * ── MAVLink v1 frame layout ──────────────────────────────────────────
 *   [0xFE][LEN][SEQ][SYSID][COMPID][MSGID]  ← 6-byte header
 *   [payload 0 … LEN-1]                      ← LEN bytes
 *   [CKS_LO][CKS_HI]                         ← 2-byte CRC
 *
 * ── CRC coverage ─────────────────────────────────────────────────────
 *   Bytes [1..5+LEN]  (i.e., LEN through end of payload)
 *   plus a per-message CRC_EXTRA byte appended at end of CRC computation.
 *
 * ── Endianness ───────────────────────────────────────────────────────
 *   MAVLink is little-endian. Cortex-M4 is little-endian.
 *   memcpy from struct → frame payload is correct with no byte-swapping.
 */

#ifndef FC_MAVLINK_MINIMAL_H
#define FC_MAVLINK_MINIMAL_H

#include <stdint.h>
#include <string.h>

/* ── MAVLink v1 constants ─────────────────────────────────────────── */
#define MAVLINK_STX         0xFEU
#define MAVLINK_SYSID       1U
#define MAVLINK_COMPID      1U

/* Message IDs */
#define MAVLINK_MSG_ID_HEARTBEAT       0U
#define MAVLINK_MSG_ID_SYS_STATUS      1U
#define MAVLINK_MSG_ID_GPS_RAW_INT    24U
#define MAVLINK_MSG_ID_ATTITUDE       30U
#define MAVLINK_MSG_ID_RC_CHANNELS    65U

/* CRC_EXTRA per message (protects against wrong message version) */
#define MAVLINK_CRC_EXTRA_HEARTBEAT     50U
#define MAVLINK_CRC_EXTRA_SYS_STATUS   124U
#define MAVLINK_CRC_EXTRA_GPS_RAW_INT   24U
#define MAVLINK_CRC_EXTRA_ATTITUDE      39U
#define MAVLINK_CRC_EXTRA_RC_CHANNELS  118U

/* MAV_TYPE */
#define MAV_TYPE_QUADROTOR              2U

/* MAV_AUTOPILOT */
#define MAV_AUTOPILOT_GENERIC           0U

/* MAV_MODE_FLAG */
#define MAV_MODE_FLAG_CUSTOM_MODE_EN    0x01U
#define MAV_MODE_FLAG_STABILIZE_EN      0x10U
#define MAV_MODE_FLAG_MANUAL_INPUT_EN   0x40U
#define MAV_MODE_FLAG_SAFETY_ARMED      0x80U

/* MAV_STATE */
#define MAV_STATE_UNINIT                0U
#define MAV_STATE_BOOT                  1U
#define MAV_STATE_CALIBRATING           2U
#define MAV_STATE_STANDBY               3U
#define MAV_STATE_ACTIVE                4U
#define MAV_STATE_CRITICAL              5U
#define MAV_STATE_EMERGENCY             6U

/* GPS fix type */
#define GPS_FIX_TYPE_NO_GPS             0U
#define GPS_FIX_TYPE_NO_FIX             1U
#define GPS_FIX_TYPE_2D_FIX             2U
#define GPS_FIX_TYPE_3D_FIX             3U

/* ── CRC-16 / IBM-SDLC (X25) accumulator ─────────────────────────── */
static inline uint16_t mav_crc_acc(uint8_t d, uint16_t crc)
{
    uint8_t tmp = d ^ (uint8_t)(crc & 0xFFU);
    tmp ^= (tmp << 4U);
    return (uint16_t)((crc >> 8U)
                      ^ ((uint16_t)tmp << 8U)
                      ^ ((uint16_t)tmp << 3U)
                      ^ (tmp >> 4U));
}

/**
 * @brief  Assemble a MAVLink v1 frame into @p frame buffer.
 *
 * @param  frame       Destination buffer — must be ≥ (6 + payload_len + 2) bytes.
 * @param  msg_id      Message ID (use MAVLINK_MSG_ID_* constants).
 * @param  crc_extra   Per-message magic byte (use MAVLINK_CRC_EXTRA_* constants).
 * @param  payload     Pointer to packed payload struct.
 * @param  payload_len Payload size in bytes.
 * @param  seq         Pointer to sequence counter — auto-incremented.
 * @return Total frame length in bytes (header + payload + CRC).
 */
static inline uint8_t mavlink_pack(uint8_t *frame,
                                    uint8_t  msg_id,
                                    uint8_t  crc_extra,
                                    const void *payload,
                                    uint8_t  payload_len,
                                    uint8_t *seq)
{
    frame[0] = MAVLINK_STX;
    frame[1] = payload_len;
    frame[2] = (*seq)++;
    frame[3] = MAVLINK_SYSID;
    frame[4] = MAVLINK_COMPID;
    frame[5] = msg_id;
    memcpy(&frame[6], payload, payload_len);

    /* CRC over frame[1..5+payload_len], then CRC_EXTRA */
    uint16_t crc = 0xFFFFU;
    for (uint8_t i = 1U; i <= (uint8_t)(5U + payload_len); i++)
        crc = mav_crc_acc(frame[i], crc);
    crc = mav_crc_acc(crc_extra, crc);

    frame[6U + payload_len]     = (uint8_t)(crc & 0xFFU);
    frame[6U + payload_len + 1U] = (uint8_t)(crc >> 8U);

    return (uint8_t)(6U + payload_len + 2U);
}

/* ── Packed payload structs ───────────────────────────────────────── */

#pragma pack(push, 1)

/** HEARTBEAT — msg_id 0, 9 bytes */
typedef struct {
    uint32_t custom_mode;
    uint8_t  type;
    uint8_t  autopilot;
    uint8_t  base_mode;
    uint8_t  system_status;
    uint8_t  mavlink_version;
} mav_heartbeat_t;

/** SYS_STATUS — msg_id 1, 31 bytes */
typedef struct {
    uint32_t sensors_present;
    uint32_t sensors_enabled;
    uint32_t sensors_health;
    uint16_t load;               /* CPU load 0–1000 (= 0–100.0 %) */
    uint16_t voltage_battery;    /* mV */
    int16_t  current_battery;    /* cA; -1 = unknown */
    uint16_t drop_rate_comm;
    uint16_t errors_comm;
    uint16_t errors_count1;
    uint16_t errors_count2;
    uint16_t errors_count3;
    uint16_t errors_count4;
    int8_t   battery_remaining;  /* %; -1 = unknown */
} mav_sys_status_t;

/** GPS_RAW_INT — msg_id 24, 30 bytes */
typedef struct {
    uint64_t time_usec;
    int32_t  lat;                /* 1e-7 degrees */
    int32_t  lon;                /* 1e-7 degrees */
    int32_t  alt;                /* mm above MSL */
    uint16_t eph;                /* HDOP × 100; UINT16_MAX = unknown */
    uint16_t epv;                /* VDOP × 100 */
    uint16_t vel;                /* cm/s */
    uint16_t cog;                /* course over ground × 100 deg */
    uint8_t  fix_type;
    uint8_t  satellites_visible;
} mav_gps_raw_int_t;

/** ATTITUDE — msg_id 30, 28 bytes */
typedef struct {
    uint32_t time_boot_ms;
    float    roll;          /* rad */
    float    pitch;         /* rad */
    float    yaw;           /* rad */
    float    rollspeed;     /* rad/s */
    float    pitchspeed;    /* rad/s */
    float    yawspeed;      /* rad/s */
} mav_attitude_t;

/** RC_CHANNELS — msg_id 65, 42 bytes */
typedef struct {
    uint32_t time_boot_ms;
    uint16_t chan1_raw;
    uint16_t chan2_raw;
    uint16_t chan3_raw;
    uint16_t chan4_raw;
    uint16_t chan5_raw;
    uint16_t chan6_raw;
    uint16_t chan7_raw;
    uint16_t chan8_raw;
    uint16_t chan9_raw;
    uint16_t chan10_raw;
    uint16_t chan11_raw;
    uint16_t chan12_raw;
    uint16_t chan13_raw;
    uint16_t chan14_raw;
    uint16_t chan15_raw;
    uint16_t chan16_raw;
    uint16_t chan17_raw;
    uint16_t chan18_raw;
    uint8_t  chancount;
    uint8_t  rssi;
} mav_rc_channels_t;

#pragma pack(pop)

/* Compile-time size assertions */
typedef char _mav_chk_heartbeat [(sizeof(mav_heartbeat_t)   ==  9) ? 1 : -1];
typedef char _mav_chk_sys_status[(sizeof(mav_sys_status_t)  == 31) ? 1 : -1];
typedef char _mav_chk_gps       [(sizeof(mav_gps_raw_int_t) == 30) ? 1 : -1];
typedef char _mav_chk_attitude  [(sizeof(mav_attitude_t)    == 28) ? 1 : -1];
typedef char _mav_chk_rc        [(sizeof(mav_rc_channels_t) == 42) ? 1 : -1];

#endif /* FC_MAVLINK_MINIMAL_H */
