/**
 * @file    gps.h
 * @brief   GPS NMEA parser — USART6 / PC6 RX / PC7 TX
 *
 * ── NMEA sentences parsed ────────────────────────────────────────────
 *   $GPGGA / $GNGGA — lat, lon, fix quality, satellites, HDOP, altitude
 *   $GPRMC / $GNRMC — lat, lon, speed (knots), course, UTC time
 *
 *   The GN-prefix variants are produced by multi-constellation modules
 *   (u-blox M8/M9).  Both GP and GN prefixes are accepted.
 *
 * ── NMEA checksum ────────────────────────────────────────────────────
 *   XOR of all bytes between '$' and '*' (exclusive).
 *   Frames failing the checksum are silently discarded.
 *
 * ── DMA strategy ─────────────────────────────────────────────────────
 *   USART6_RX → DMA2 Stream1 Channel5  (RM0090 Table 42)
 *   256-byte circular DMA buffer.  IDLE-line interrupt signals end of
 *   NMEA burst.  GPS_Process() (call from main loop / ≤10 Hz task)
 *   scans from the last-read position to the DMA write pointer, feeds
 *   bytes into a sentence accumulator, and parses on \n.
 *
 * ── Baud rate ────────────────────────────────────────────────────────
 *   Default : 9600 baud (standard GPS modules)
 *   High-speed: many u-blox modules support 115200 — send UBX-CFG-PRT
 *   to switch.  Update GPS_BAUD accordingly and call GPS_Init() again.
 *
 * ── Usage ─────────────────────────────────────────────────────────────
 *   GPS_Init();
 *   // In a ≤10 Hz task:
 *   GPS_Process();
 *   if (GPS_IsValid()) {
 *       const gps_data_t *fix = GPS_GetData();
 *       use fix->latitude, fix->longitude, fix->altitude_m ...
 *   }
 *
 * ── ISR wiring (stm32f4xx_it.c) ──────────────────────────────────────
 *   void USART6_IRQHandler(void)       { GPS_USART_IRQHandler(); }
 *   void DMA2_Stream1_IRQHandler(void) { GPS_DMA_IRQHandler();   }
 */

#ifndef FC_GPS_H
#define FC_GPS_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* ── Buffer / sentence constants ─────────────────────────────────── */
#define GPS_DMA_BUF_LEN       256U   /**< Circular DMA receive buffer  */
#define GPS_SENTENCE_MAX_LEN  128U   /**< Max chars per NMEA sentence  */
#define GPS_FIX_TIMEOUT_MS   2000U   /**< Signal lost after N ms       */

/* ── Fix quality values (GGA field 6) ───────────────────────────── */
#define GPS_FIX_NONE          0U
#define GPS_FIX_GPS           1U
#define GPS_FIX_DGPS          2U

/* ── Data structure ─────────────────────────────────────────────── */

/**
 * @brief  Decoded GPS fix — updated on every valid GGA + RMC frame pair.
 */
typedef struct {
    double   latitude;        /**< Decimal degrees, + = North          */
    double   longitude;       /**< Decimal degrees, + = East           */
    float    altitude_m;      /**< Altitude MSL in metres              */
    float    speed_mps;       /**< Ground speed in m/s (from RMC)      */
    float    course_deg;      /**< True course in degrees (from RMC)   */
    uint8_t  fix;             /**< Fix quality: 0=none, 1=GPS, 2=DGPS */
    uint8_t  satellites;      /**< Satellites used in solution         */
    float    hdop;            /**< Horizontal dilution of precision    */
    uint8_t  hour;            /**< UTC hour   (from RMC/GGA)           */
    uint8_t  minute;          /**< UTC minute                          */
    uint8_t  second;          /**< UTC second                          */
    uint32_t last_fix_ms;     /**< HAL tick of last valid fix          */
    uint32_t frame_count;     /**< Total valid sentences parsed        */
} gps_data_t;

/* ── Public API ─────────────────────────────────────────────────── */

/**
 * @brief  Init PC6/PC7 (AF8), USART6 (9600/8N1), DMA2-Stream1,
 *         IDLE-line interrupt.  Reception starts immediately.
 */
void GPS_Init(void);

/**
 * @brief  Scan DMA buffer for complete NMEA sentences and parse them.
 *         Call from main loop or a ≤10 Hz task.
 *         Sentences parsed: GGA (position/fix) and RMC (speed/course/time).
 */
void GPS_Process(void);

/**
 * @brief  Return pointer to the most recently decoded fix.
 *         Data is stale until at least one valid GGA sentence is parsed.
 */
const gps_data_t *GPS_GetData(void);

/**
 * @brief  Returns true if GPS has a valid fix and data is fresh.
 *         Combines fix quality check with a 2-second staleness timeout.
 */
bool GPS_IsValid(void);

/**
 * @brief  USART6 global ISR — call from stm32f4xx_it.c:
 *           void USART6_IRQHandler(void) { GPS_USART_IRQHandler(); }
 */
void GPS_USART_IRQHandler(void);

/**
 * @brief  DMA2 Stream1 ISR — call from stm32f4xx_it.c:
 *           void DMA2_Stream1_IRQHandler(void) { GPS_DMA_IRQHandler(); }
 */
void GPS_DMA_IRQHandler(void);

#endif /* FC_GPS_H */
