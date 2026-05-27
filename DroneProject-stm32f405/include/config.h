/**
 * @file    config.h
 * @brief   Global firmware configuration — tune parameters here
 */

#ifndef FC_CONFIG_H
#define FC_CONFIG_H

/* ------------------------------------------------------------------ */
/*  Control loop rates                                                 */
/* ------------------------------------------------------------------ */
#define MAIN_LOOP_RATE_HZ       1000U   /* inner PID loop            */
#define ATTITUDE_RATE_HZ        1000U   /* AHRS / Madgwick update    */
#define GPS_PARSE_RATE_HZ         10U   /* GPS sentence processing   */
#define TELEMETRY_RATE_HZ         10U   /* MAVLink heartbeat / data  */
#define BLACKBOX_RATE_HZ         100U   /* SD card logging           */

/* ------------------------------------------------------------------ */
/*  PID defaults (tune in flight)                                      */
/* ------------------------------------------------------------------ */
#define PID_ROLL_RATE_KP        0.15f
#define PID_ROLL_RATE_KI        0.10f
#define PID_ROLL_RATE_KD        0.004f

#define PID_PITCH_RATE_KP       0.15f
#define PID_PITCH_RATE_KI       0.10f
#define PID_PITCH_RATE_KD       0.004f

#define PID_YAW_RATE_KP         0.20f
#define PID_YAW_RATE_KI         0.05f
#define PID_YAW_RATE_KD         0.00f

#define PID_ROLL_ANGLE_KP       5.0f
#define PID_PITCH_ANGLE_KP      5.0f

/* ------------------------------------------------------------------ */
/*  AHRS                                                               */
/* ------------------------------------------------------------------ */
#define MADGWICK_BETA           0.1f    /* filter gain (0.0 – 1.0)   */

/* ------------------------------------------------------------------ */
/*  RC / SBUS                                                          */
/* ------------------------------------------------------------------ */
#define SBUS_FAILSAFE_TIMEOUT_MS  500U  /* ms without valid frame     */
#define RC_THROTTLE_ARM_THRESHOLD 200U  /* raw SBUS value (< this to arm) */
#define RC_ARM_CHANNEL            4U    /* channel index 0-based      */

/* Stick → setpoint scaling */
#define MAX_ROLL_ANGLE_DEG        30.0f  /* ±degrees at full stick     */
#define MAX_PITCH_ANGLE_DEG       30.0f
#define MAX_YAW_RATE_DPS         200.0f  /* deg/s at full yaw stick    */
#define RC_DEADBAND_US            20U    /* µs deadband around 1500    */
#define THROTTLE_IDLE_US         1050U   /* below → ground, freeze I   */

/* ------------------------------------------------------------------ */
/*  Battery monitoring                                                 */
/* ------------------------------------------------------------------ */
#define BATTERY_CELLS           3U      /* 3S LiPo                   */
#define BATTERY_LOW_VOLT_MV     10500U  /* 3.5 V/cell × 3            */
#define BATTERY_CRIT_VOLT_MV     9900U  /* 3.3 V/cell × 3            */

/* ------------------------------------------------------------------ */
/*  Debug                                                              */
/* ------------------------------------------------------------------ */
/* #define FC_DEBUG_UART */   /* Uncomment to enable debug UART output */

#endif /* FC_CONFIG_H */
