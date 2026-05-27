/**
 * @file    usb_device.h
 * @brief   Top-level USB Device init — call once from main()
 */

#ifndef __USB_DEVICE_H
#define __USB_DEVICE_H

#include "usbd_def.h"

/** Device handle — used by syscalls.c to check USBD_STATE_CONFIGURED */
extern USBD_HandleTypeDef hUsbDeviceFS;

/**
 * @brief  Initialise the full USB CDC stack:
 *         USBD_Init → RegisterClass(CDC) → CDC_RegisterInterface → Start
 */
void USB_Device_Init(void);

#endif /* __USB_DEVICE_H */
