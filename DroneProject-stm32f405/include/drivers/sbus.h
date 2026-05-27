/**
 * @file    sbus.h
 * @brief   SBUS RC receiver driver — USART1 / PA10 / DMA2-Stream5
 *
 * ── SBUS protocol specification ──────────────────────────────────────
 *   Baud rate  : 100,000 bps
 *   Data bits  : 8
 *   Parity     : Even
 *   Stop bits  : 2
 *   Logic      : INVERTED (mark=0V, space=3.3V) — external 74HC04/transistor inverter
 *
 * ── Frame layout (25 bytes, 3 ms @ 100k baud) ────────────────────────
 *   Byte  0    : Start byte  = 0x0F
 *   Bytes 1–22 : 16 × 11-bit channels, packed little-endian (LSB first)
 *   Byte  23   : Flags
 *                  bit 0 = CH17 digital
 *                  bit 1 = CH18 digital
 *                  bit 2 = Frame Lost  (single frame dropped)
 *                  bit 3 = Failsafe    (RC link lost, holds last known)
 *   Byte  24   : End byte    = 0x00
 *
 * ── Channel value range ───────────────────────────────────────────────
 *   Raw 11-bit : 172 (min)  –  1024 (centre)  –  1811 (max)
 *   Mapped µs  : 1000       –  1500           –  2000
 *
 *   Formula: µs = 1000 + (raw - 172) * 1000 / (1811 - 172)
 *                       = 1000 + (raw - 172) * 1000 / 1639
 *
 * ── DMA strategy ─────────────────────────────────────────────────────
 *   USART1_RX → DMA2 Stream5 Channel4 — circular, 50-byte double buffer.
 *   An idle-line interrupt (USART_IT_IDLE) fires when the bus is silent
 *   after a frame.  The ISR copies the relevant 25 bytes into a
 *   staging buffer and sets a parse flag for the main loop.
 *
 * ── Frame rate ────────────────────────────────────────────────────────
 *   FrSky / Futaba SBUS: 7 ms (high speed) or 14 ms (normal)
 *   FrSky SBUS Fast    : 4 ms
 *   SBUS_IsValid() considers signal lost after SBUS_FAILSAFE_TIMEOUT_MS.
 *
 * ── Usage ─────────────────────────────────────────────────────────────
 *   SBUS_Init();                  // once, after SystemClock_Config
 *   // In main loop or 50 Hz task:
 *   if (SBUS_Parse()) {           // returns 1 when new frame ready
 *       const sbus_data_t *rc = SBUS_GetData();
 *       uint16_t thr_us = SBUS_ToMicros(rc->channel[2]);  // throttle
 *       if (!SBUS_IsValid()) { Motor_StopAll(); }
 *   }
 */

#ifndef FC_SBUS_H
#define FC_SBUS_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* ── Protocol constants ──────────────────────────────────────────── */
#define SBUS_FRAME_LEN          25U    /**< Bytes per SBUS frame       */
#define SBUS_START_BYTE         0x0FU  /**< First byte of every frame  */
#define SBUS_END_BYTE           0x00U  /**< Last  byte of every frame  */
#define SBUS_CHANNELS           16U    /**< Analogue channels          */

/* Raw 11-bit channel range */
#define SBUS_CH_MIN             172U
#define SBUS_CH_MID            1024U
#define SBUS_CH_MAX            1811U

/* Flags byte (byte 23) bit masks */
#define SBUS_FLAG_CH17          (1U << 0U)  /**< Digital channel 17   */
#define SBUS_FLAG_CH18          (1U << 1U)  /**< Digital channel 18   */
#define SBUS_FLAG_FRAME_LOST    (1U << 2U)  /**< Single frame dropped */
#define SBUS_FLAG_FAILSAFE      (1U << 3U)  /**< RC link lost         */

/* DMA buffer: double-buffer → 2 × frame length */
#define SBUS_DMA_BUF_LEN        (SBUS_FRAME_LEN * 2U)

/* Mapped µs output range */
#define SBUS_US_MIN             1000U
#define SBUS_US_MAX             2000U

/* ── Channel index aliases (0-based) ─────────────────────────────── */
#define SBUS_CH_AILERON         0U   /**< Roll   */
#define SBUS_CH_ELEVATOR        1U   /**< Pitch  */
#define SBUS_CH_THROTTLE        2U   /**< Throttle (arm when low) */
#define SBUS_CH_RUDDER          3U   /**< Yaw    */
#define SBUS_CH_ARM             4U   /**< Arming switch          */
#define SBUS_CH_MODE            5U   /**< Flight mode            */

/* ── Data structures ─────────────────────────────────────────────── */

/**
 * @brief  Decoded SBUS frame — updated on every valid received frame.
 */
typedef struct {
    uint16_t channel[SBUS_CHANNELS]; /**< Raw 11-bit values [172–1811]   */
    bool     ch17;                   /**< Digital channel 17             */
    bool     ch18;                   /**< Digital channel 18             */
    bool     frame_lost;             /**< Single frame was dropped       */
    bool     failsafe;               /**< RC transmitter link lost       */
    uint32_t last_frame_ms;          /**< HAL tick of last valid frame   */
    uint32_t frame_count;            /**< Total frames received          */
    uint32_t error_count;            /**< Frames with bad start/end byte */
} sbus_data_t;

/* ── Public API ──────────────────────────────────────────────────── */

/**
 * @brief  Initialise PA10 (AF7), USART1 (100k/8E2), DMA2-Stream5,
 *         and IDLE-line interrupt.  Reception begins immediately.
 */
void SBUS_Init(void);

/**
 * @brief  Check if a new frame has arrived and decode it.
 *         Call from main loop or a ≥50 Hz task.
 * @return true  New frame decoded — sbus_data_t updated.
 * @return false No new frame since last call.
 */
bool SBUS_Parse(void);

/**
 * @brief  Get pointer to the most recently decoded frame data.
 *         Data is valid only after at least one successful SBUS_Parse().
 */
const sbus_data_t *SBUS_GetData(void);

/**
 * @brief  Returns true if signal is healthy:
 *           – failsafe bit NOT set
 *           – last frame was < SBUS_FAILSAFE_TIMEOUT_MS ago
 */
bool SBUS_IsValid(void);

/**
 * @brief  Map a raw 11-bit SBUS value to pulse width in microseconds.
 *         Input is clamped to [SBUS_CH_MIN, SBUS_CH_MAX] before mapping.
 *
 *   µs = SBUS_US_MIN + (raw - SBUS_CH_MIN) * (SBUS_US_MAX - SBUS_US_MIN)
 *                                           / (SBUS_CH_MAX - SBUS_CH_MIN)
 *      = 1000 + (raw - 172) * 1000 / 1639
 *
 * @param  raw  Raw SBUS channel value.
 * @return Pulse width in µs, range [1000, 2000].
 */
uint16_t SBUS_ToMicros(uint16_t raw);

/**
 * @brief  Map all 16 channels at once into a pre-allocated array.
 * @param  out  Array of at least SBUS_CHANNELS uint16_t.
 */
void SBUS_AllToMicros(uint16_t out[SBUS_CHANNELS]);

/**
 * @brief  USART1 global ISR — call from stm32f4xx_it.c:
 *           void USART1_IRQHandler(void) { SBUS_USART_IRQHandler(); }
 */
void SBUS_USART_IRQHandler(void);

/**
 * @brief  DMA2 Stream5 ISR — call from stm32f4xx_it.c:
 *           void DMA2_Stream5_IRQHandler(void) { SBUS_DMA_IRQHandler(); }
 */
void SBUS_DMA_IRQHandler(void);

#endif /* FC_SBUS_H */
