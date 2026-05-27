/**
 * @file    usbd_desc.h
 * @brief   USB device / configuration / string descriptors
 */

#ifndef __USBD_DESC_H
#define __USBD_DESC_H

#include "usbd_def.h"

/* VID = 0x0483 (STMicro), PID = 0x5740 (CDC Virtual COM Port) */
#define USBD_VID                             0x0483U
#define USBD_PID_FS                          0x5740U
#define USBD_LANGID_STRING                   0x0409U  /* English (US) */
#define USBD_MANUFACTURER_STRING             "STM32 FC"
#define USBD_PRODUCT_STRING_FS               "FC Virtual COM Port"
#define USBD_SERIALNUMBER_STRING_FS          "00000000001A"
#define USBD_CONFIGURATION_STRING_FS         "CDC Config"
#define USBD_INTERFACE_STRING_FS             "CDC Interface"

/** Descriptor table — passed to USBD_Init() */
extern USBD_DescriptorsTypeDef FS_Desc;

#endif /* __USBD_DESC_H */
