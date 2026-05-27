/**
 * @file    usbd_desc.c
 * @brief   USB Device / Configuration / String descriptors
 *
 * Device:  VID=0x0483 (STMicro), PID=0x5740 (CDC Virtual COM Port)
 * Class:   CDC (02/00/00) — single function, no IAD required
 *
 * Config descriptor layout (67 bytes total):
 *   [09] Configuration
 *   [09] CDC Communication Interface
 *   [05]   CDC Header Functional Descriptor
 *   [05]   CDC Call Management Functional Descriptor
 *   [04]   CDC ACM Functional Descriptor
 *   [05]   CDC Union Functional Descriptor
 *   [07]   Command endpoint (EP2 IN Interrupt)
 *   [09] CDC Data Interface
 *   [07]   Data OUT endpoint (EP1 OUT Bulk)
 *   [07]   Data IN  endpoint (EP1 IN  Bulk)
 *        = 67 bytes
 */

#include "usb/usbd_desc.h"
#include "usb/usbd_conf.h"
#include "usbd_core.h"
#include <string.h>

/* ── Sizes ────────────────────────────────────────────────────────── */
#define USB_LEN_DEV_DESC          18U
#define USB_LEN_LANGID_STR_DESC    4U
#define USB_CDC_CONFIG_DESC_SIZ   67U
#define MAX_STR_UNICODE_SZ        64U

/* ── Device descriptor ───────────────────────────────────────────── */
static uint8_t s_DevDesc[USB_LEN_DEV_DESC] = {
    USB_LEN_DEV_DESC,       /* bLength             */
    0x01U,                  /* bDescriptorType     = DEVICE */
    0x00U, 0x02U,           /* bcdUSB              = 2.00  */
    0x02U,                  /* bDeviceClass        = CDC   */
    0x00U,                  /* bDeviceSubClass             */
    0x00U,                  /* bDeviceProtocol             */
    0x40U,                  /* bMaxPacketSize0     = 64    */
    LOBYTE(USBD_VID),       /* idVendor low                */
    HIBYTE(USBD_VID),       /* idVendor high               */
    LOBYTE(USBD_PID_FS),    /* idProduct low               */
    HIBYTE(USBD_PID_FS),    /* idProduct high              */
    0x00U, 0x02U,           /* bcdDevice           = 2.00  */
    USBD_IDX_MFC_STR,       /* iManufacturer       = 1     */
    USBD_IDX_PRODUCT_STR,   /* iProduct            = 2     */
    USBD_IDX_SERIAL_STR,    /* iSerialNumber       = 3     */
    0x01U                   /* bNumConfigurations  = 1     */
};

/* ── Language ID string ──────────────────────────────────────────── */
static uint8_t s_LangIDDesc[USB_LEN_LANGID_STR_DESC] = {
    USB_LEN_LANGID_STR_DESC,
    0x03U,                          /* bDescriptorType = STRING */
    LOBYTE(USBD_LANGID_STRING),
    HIBYTE(USBD_LANGID_STRING)
};

/* ── Configuration descriptor (67 bytes) ────────────────────────── */
static uint8_t s_CfgDesc[USB_CDC_CONFIG_DESC_SIZ] = {
    /* ── Configuration ── */
    0x09U, 0x02U,
    LOBYTE(USB_CDC_CONFIG_DESC_SIZ),
    HIBYTE(USB_CDC_CONFIG_DESC_SIZ),
    0x02U,   /* bNumInterfaces    = 2  */
    0x01U,   /* bConfigurationValue   */
    0x00U,   /* iConfiguration    (none) */
    0xC0U,   /* bmAttributes: self-powered */
    0x32U,   /* bMaxPower: 100 mA */

    /* ── CDC Communication Interface ── */
    0x09U, 0x04U, 0x00U, 0x00U, 0x01U, 0x02U, 0x02U, 0x01U, 0x00U,

    /* ── CDC Header Functional Descriptor ── */
    0x05U, 0x24U, 0x00U, 0x10U, 0x01U,

    /* ── CDC Call Management Functional Descriptor ── */
    0x05U, 0x24U, 0x01U, 0x00U, 0x01U,

    /* ── CDC ACM Functional Descriptor ── */
    0x04U, 0x24U, 0x02U, 0x02U,

    /* ── CDC Union Functional Descriptor ── */
    0x05U, 0x24U, 0x06U, 0x00U, 0x01U,

    /* ── Command EP (EP2 IN, Interrupt, 8 bytes, 16 ms) ── */
    0x07U, 0x05U, CDC_CMD_EP, 0x03U,
    LOBYTE(CDC_CMD_PACKET_SIZE), HIBYTE(CDC_CMD_PACKET_SIZE),
    0x10U,

    /* ── CDC Data Interface ── */
    0x09U, 0x04U, 0x01U, 0x00U, 0x02U, 0x0AU, 0x00U, 0x00U, 0x00U,

    /* ── Data OUT EP (EP1 OUT, Bulk, 64 bytes) ── */
    0x07U, 0x05U, CDC_DATA_FS_OUT_EP, 0x02U,
    LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE), HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
    0x00U,

    /* ── Data IN EP (EP1 IN, Bulk, 64 bytes) ── */
    0x07U, 0x05U, CDC_DATA_FS_IN_EP, 0x02U,
    LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE), HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
    0x00U
};

/* ── String builder (ASCII → UTF-16LE descriptor) ─────────────────── */
static uint8_t s_StrBuf[MAX_STR_UNICODE_SZ];

static uint8_t *make_string(const char *str, uint16_t *len)
{
    uint8_t n = (uint8_t)strlen(str);
    *len = (uint16_t)(n * 2U + 2U);
    s_StrBuf[0] = (uint8_t)*len;
    s_StrBuf[1] = 0x03U;   /* bDescriptorType = STRING */
    for (uint8_t i = 0; i < n; i++) {
        s_StrBuf[2U + i * 2U]      = (uint8_t)str[i];
        s_StrBuf[2U + i * 2U + 1U] = 0x00U;
    }
    return s_StrBuf;
}

/* ── Descriptor callback implementations ────────────────────────── */

static uint8_t *USBD_FS_DeviceDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    *length = sizeof(s_DevDesc);
    return s_DevDesc;
}

static uint8_t *USBD_FS_LangIDStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    *length = sizeof(s_LangIDDesc);
    return s_LangIDDesc;
}

static uint8_t *USBD_FS_ManufacturerStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    return make_string(USBD_MANUFACTURER_STRING, length);
}

static uint8_t *USBD_FS_ProductStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    return make_string(USBD_PRODUCT_STRING_FS, length);
}

static uint8_t *USBD_FS_SerialStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    return make_string(USBD_SERIALNUMBER_STRING_FS, length);
}

static uint8_t *USBD_FS_ConfigStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    return make_string(USBD_CONFIGURATION_STRING_FS, length);
}

static uint8_t *USBD_FS_InterfaceStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    return make_string(USBD_INTERFACE_STRING_FS, length);
}

static uint8_t *USBD_FS_GetCfgDesc(uint16_t *length)
{
    *length = sizeof(s_CfgDesc);
    return s_CfgDesc;
}

/* ── Public descriptor table ─────────────────────────────────────── */
USBD_DescriptorsTypeDef FS_Desc = {
    USBD_FS_DeviceDescriptor,
    USBD_FS_LangIDStrDescriptor,
    USBD_FS_ManufacturerStrDescriptor,
    USBD_FS_ProductStrDescriptor,
    USBD_FS_SerialStrDescriptor,
    USBD_FS_ConfigStrDescriptor,
    USBD_FS_InterfaceStrDescriptor,
};
