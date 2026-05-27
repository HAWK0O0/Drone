/**
 * @file    usbd_conf.h
 * @brief   USB Device library configuration — USB OTG FS (PA11/PA12)
 *
 * This file provides the compile-time constants required by the
 * STM32 USB Device Library (ST Middleware).
 */

#ifndef __USBD_CONF_H
#define __USBD_CONF_H

#include "stm32f4xx_hal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ── Device class limits ─────────────────────────────────────────── */
#define USBD_MAX_NUM_INTERFACES              1U
#define USBD_MAX_NUM_CONFIGURATION           1U
#define USBD_MAX_SUPPORTED_CLASS             1U
#define USBD_MAX_CLASS_ENDPOINTS             3U
#define USBD_MAX_CLASS_INTERFACES            2U

/* ── Debug level (0 = silent) ────────────────────────────────────── */
#define USBD_DEBUG_LEVEL                     0U

/* ── CDC endpoint addresses ─────────────────────────────────────── */
#define CDC_CMD_EP                           0x82U   /* CDC command IN  */
#define CDC_DATA_FS_IN_EP                    0x81U   /* bulk data   IN  */
#define CDC_DATA_FS_OUT_EP                   0x01U   /* bulk data   OUT */
#define CDC_DATA_HS_IN_EP                    0x81U
#define CDC_DATA_HS_OUT_EP                   0x01U

/* ── CDC packet sizes ────────────────────────────────────────────── */
#define CDC_CMD_PACKET_SIZE                  8U
#define CDC_DATA_FS_MAX_PACKET_SIZE          64U
#define CDC_DATA_HS_MAX_PACKET_SIZE          512U

/* ── Memory / debug macros ───────────────────────────────────────── */
#define USBD_malloc                          malloc
#define USBD_free                            free
#define USBD_memset                          memset
#define USBD_memcpy                          memcpy
#define USBD_Delay                           HAL_Delay

#if (USBD_DEBUG_LEVEL > 0)
#define USBD_UsrLog(...)  do { printf(__VA_ARGS__); printf("\n"); } while(0)
#else
#define USBD_UsrLog(...)  do {} while(0)
#endif

#if (USBD_DEBUG_LEVEL > 1)
#define USBD_ErrLog(...)  do { printf(__VA_ARGS__); printf("\n"); } while(0)
#else
#define USBD_ErrLog(...)  do {} while(0)
#endif

#if (USBD_DEBUG_LEVEL > 2)
#define USBD_DbgLog(...)  do { printf(__VA_ARGS__); printf("\n"); } while(0)
#else
#define USBD_DbgLog(...)  do {} while(0)
#endif

#endif /* __USBD_CONF_H */
