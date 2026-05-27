/**
 * @file    usb_device.c
 * @brief   Top-level USB Device initialisation
 *
 * Call USB_Device_Init() once from main() after SystemClock_Config().
 * After this call, the MCU enumerates as a USB CDC Virtual COM Port.
 */

#include "usb/usb_device.h"
#include "usb/usbd_desc.h"
#include "usb/usbd_cdc_if.h"
#include "usbd_cdc.h"

USBD_HandleTypeDef hUsbDeviceFS;

void USB_Device_Init(void)
{
    /* 1. Initialise core with FS descriptors */
    USBD_Init(&hUsbDeviceFS, &FS_Desc, 0U);

    /* 2. Register CDC class */
    USBD_RegisterClass(&hUsbDeviceFS, &USBD_CDC);

    /* 3. Register application callbacks (TX/RX buffers, control handler) */
    USBD_CDC_RegisterInterface(&hUsbDeviceFS, &USBD_Interface_fops_FS);

    /* 4. Start the USB stack — device now appears on the bus */
    USBD_Start(&hUsbDeviceFS);
}
