/**
 * @file    usbd_cdc_if.h
 * @brief   CDC interface — application TX/RX callbacks and transmit API
 */

#ifndef __USBD_CDC_IF_H
#define __USBD_CDC_IF_H

#include "usbd_cdc.h"

#define APP_RX_DATA_SIZE   256U   /* receive ring-buffer size (bytes) */
#define APP_TX_DATA_SIZE   256U   /* transmit staging buffer  (bytes) */

/** Interface function-pointer table registered with the CDC class */
extern USBD_CDC_ItfTypeDef USBD_Interface_fops_FS;

/**
 * @brief  Transmit data over USB CDC (best-effort — returns USBD_BUSY if
 *         a previous transfer is still in flight).
 * @param  Buf   Pointer to data buffer
 * @param  Len   Number of bytes to send
 * @return USBD_OK / USBD_BUSY / USBD_FAIL
 */
uint8_t CDC_Transmit_FS(uint8_t *Buf, uint16_t Len);

#endif /* __USBD_CDC_IF_H */
