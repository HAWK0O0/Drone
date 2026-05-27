/**
 * @file    usbd_cdc_if.c
 * @brief   CDC application interface — TX/RX callbacks and transmit API
 *
 * Data flow:
 *   printf() → _write() → CDC_Transmit_FS()
 *       ↓                       ↑
 *   UserTxBufferFS ──────────── (copies data here for DMA safety)
 *
 *   USB host → CDC_Receive_FS() → UserRxBufferFS (ring storage)
 */

#include "usb/usbd_cdc_if.h"
#include "usb/usb_device.h"

/* ── Buffers ─────────────────────────────────────────────────────── */
static uint8_t UserRxBufferFS[APP_RX_DATA_SIZE];
static uint8_t UserTxBufferFS[APP_TX_DATA_SIZE];

/* ── Prototype declarations (required by USBD_Interface_fops_FS) ─── */
static int8_t CDC_Init_FS(void);
static int8_t CDC_DeInit_FS(void);
static int8_t CDC_Control_FS(uint8_t cmd, uint8_t *pbuf, uint16_t length);
static int8_t CDC_Receive_FS(uint8_t *pbuf, uint32_t *Len);

/* ── Interface registration table ───────────────────────────────── */
USBD_CDC_ItfTypeDef USBD_Interface_fops_FS = {
    CDC_Init_FS,
    CDC_DeInit_FS,
    CDC_Control_FS,
    CDC_Receive_FS
};

/* ── Callbacks ───────────────────────────────────────────────────── */

static int8_t CDC_Init_FS(void)
{
    USBD_CDC_SetTxBuffer(&hUsbDeviceFS, UserTxBufferFS, 0U);
    USBD_CDC_SetRxBuffer(&hUsbDeviceFS, UserRxBufferFS);
    return USBD_OK;
}

static int8_t CDC_DeInit_FS(void)
{
    return USBD_OK;
}

/**
 * @brief  Handle CDC control requests (line coding, serial state, etc.).
 *         We acknowledge all requests but don't configure any real UART.
 */
static int8_t CDC_Control_FS(uint8_t cmd, uint8_t *pbuf, uint16_t length)
{
    (void)length;

    /* Line coding defaults — host reads these as 115200 8N1 */
    static uint8_t s_line_coding[7] = {
        0x00U, 0xC2U, 0x01U, 0x00U,  /* dwDTERate   = 115200 bps */
        0x00U,                         /* bCharFormat = 1 stop bit */
        0x00U,                         /* bParityType = None       */
        0x08U                          /* bDataBits   = 8          */
    };

    switch (cmd) {
    case 0x20U: /* SET_LINE_CODING  */
        if (pbuf && length == 7U) {
            for (uint8_t i = 0; i < 7U; i++) s_line_coding[i] = pbuf[i];
        }
        break;
    case 0x21U: /* GET_LINE_CODING  */
        if (pbuf) {
            for (uint8_t i = 0; i < 7U; i++) pbuf[i] = s_line_coding[i];
        }
        break;
    default:
        break;
    }
    return USBD_OK;
}

/**
 * @brief  Data received from USB host — re-arm RX after each packet.
 *         Application can hook here for a command parser if needed.
 */
static int8_t CDC_Receive_FS(uint8_t *pbuf, uint32_t *Len)
{
    (void)pbuf;
    (void)Len;
    /* Re-arm the OUT endpoint to accept the next packet */
    USBD_CDC_SetRxBuffer(&hUsbDeviceFS, UserRxBufferFS);
    USBD_CDC_ReceivePacket(&hUsbDeviceFS);
    return USBD_OK;
}

/* ── Public TX API ───────────────────────────────────────────────── */

/**
 * @brief  Send up to APP_TX_DATA_SIZE bytes over USB CDC.
 *         Copies Buf into the internal static buffer (safe for DMA).
 *         Returns USBD_BUSY if a previous transfer is in progress.
 */
uint8_t CDC_Transmit_FS(uint8_t *Buf, uint16_t Len)
{
    USBD_CDC_HandleTypeDef *hcdc =
        (USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData;

    if (hcdc == NULL || hcdc->TxState != 0U) {
        return USBD_BUSY;
    }

    if (Len > APP_TX_DATA_SIZE) {
        Len = (uint16_t)APP_TX_DATA_SIZE;
    }

    /* Data must remain valid while USB DMA completes the transfer */
    for (uint16_t i = 0; i < Len; i++) {
        UserTxBufferFS[i] = Buf[i];
    }

    USBD_CDC_SetTxBuffer(&hUsbDeviceFS, UserTxBufferFS, Len);
    return USBD_CDC_TransmitPacket(&hUsbDeviceFS);
}
