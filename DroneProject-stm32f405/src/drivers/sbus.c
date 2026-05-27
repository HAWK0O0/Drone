/**
 * @file    sbus.c
 * @brief   SBUS RC receiver — USART1 / PA10 / DMA2-Stream5 / IDLE-line ISR
 *
 * ── Reception strategy ───────────────────────────────────────────────
 *
 *   DMA2-Stream5 Ch4 runs in CIRCULAR mode on the 50-byte s_dma_buf.
 *   It continuously overwrites the buffer as bytes arrive — no CPU
 *   involvement needed for individual bytes.
 *
 *   IDLE-line interrupt (fires when USART RX line stays idle for 1 frame
 *   time after activity) signals "a complete transmission burst ended".
 *   The ISR:
 *     1. Reads DMA NDTR to know where the write pointer is.
 *     2. Scans backwards for the start byte 0x0F.
 *     3. Copies the 25-byte candidate into s_rx_frame[].
 *     4. Sets s_frame_ready flag.
 *   SBUS_Parse() (called from main loop) decodes s_rx_frame[] when ready.
 *
 * ── Why IDLE-line? ───────────────────────────────────────────────────
 *   SBUS transmitters send frames back-to-back with a short gap.
 *   The IDLE interrupt reliably fires at end-of-frame without needing
 *   any timer, without knowing exact frame timing, and with zero CPU
 *   load between frames.
 *
 * ── External inverter requirement ────────────────────────────────────
 *   STM32F405 USART does NOT have hardware RX inversion (unlike F3/F7).
 *   The SBUS signal MUST pass through an external inverter
 *   (e.g. 74HC04, BC817 NPN transistor, or dedicated level-shift chip)
 *   before reaching PA10.  PA10 sees normal UART logic (mark=high).
 *
 * ── DMA resource table ───────────────────────────────────────────────
 *   USART1_RX -> DMA2, Stream5, Channel4  (RM0090 Table 42)
 *
 * ── Bit unpacking ─────────────────────────────────────────────────────
 *   SBUS packs 16 x 11-bit channels into bytes 1-22 (176 bits total),
 *   LSB first.  Each channel overlaps byte boundaries:
 *   CH0  : bits 0-10   of payload (bytes 1-2 + 3 bits of byte 3)
 *   CH1  : bits 11-21
 *   ...  etc.
 *   Unpacking loop extracts each channel by bit-shifting the payload
 *   array and masking with 0x07FF (11 bits).
 */

#include "drivers/sbus.h"
#include "pin_config.h"
#include "config.h"
#include <string.h>

/* ── Private constants ───────────────────────────────────────────────*/
#define SBUS_BAUD           100000U
#define FLAGS_BYTE_IDX      23U          /* flags byte position in frame */

/* ── Private state ───────────────────────────────────────────────────*/
static UART_HandleTypeDef  s_huart1;
static DMA_HandleTypeDef   s_hdma_rx;

/* DMA circular receive buffer — 2 x frame to always have one full frame */
static uint8_t  s_dma_buf[SBUS_DMA_BUF_LEN];

/* Staging buffer: ISR copies candidate frame here */
static uint8_t           s_rx_frame[SBUS_FRAME_LEN];
static volatile bool     s_frame_ready = false;

/* Decoded data — updated by SBUS_Parse() */
static sbus_data_t s_data = {0};

/* ── Forward declarations ────────────────────────────────────────────*/
static void sbus_gpio_init(void);
static void sbus_dma_init(void);
static void sbus_uart_init(void);
static void sbus_unpack_channels(const uint8_t *payload);

/* ══════════════════════════════════════════════════════════════════════
 *  SBUS_Init
 * ══════════════════════════════════════════════════════════════════════ */
void SBUS_Init(void)
{
    sbus_gpio_init();
    sbus_dma_init();
    sbus_uart_init();

    /* Enable IDLE-line interrupt — fires after last byte of a burst */
    __HAL_UART_ENABLE_IT(&s_huart1, UART_IT_IDLE);

    /* Kick off DMA circular reception */
    HAL_UART_Receive_DMA(&s_huart1, s_dma_buf, SBUS_DMA_BUF_LEN);
}

/* ══════════════════════════════════════════════════════════════════════
 *  SBUS_Parse  (call from main loop / 50 Hz task)
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Decode the frame staged by the ISR in s_rx_frame[].
 *
 * Frame validation:
 *   frame[0]  == 0x0F  (start byte)
 *   frame[24] == 0x00  (end byte)
 */
bool SBUS_Parse(void)
{
    if (!s_frame_ready) return false;

    /* Atomically claim the frame */
    uint8_t frame[SBUS_FRAME_LEN];
    __disable_irq();
    memcpy(frame, s_rx_frame, SBUS_FRAME_LEN);
    s_frame_ready = false;
    __enable_irq();

    /* Validate start and end bytes */
    if (frame[0] != SBUS_START_BYTE || frame[24] != SBUS_END_BYTE)
    {
        s_data.error_count++;
        return false;
    }

    /* Unpack 16 analogue channels from bytes 1-22 */
    sbus_unpack_channels(&frame[1]);

    /* Decode flags byte */
    uint8_t flags = frame[FLAGS_BYTE_IDX];
    s_data.ch17       = (flags & SBUS_FLAG_CH17)       != 0U;
    s_data.ch18       = (flags & SBUS_FLAG_CH18)       != 0U;
    s_data.frame_lost = (flags & SBUS_FLAG_FRAME_LOST) != 0U;
    s_data.failsafe   = (flags & SBUS_FLAG_FAILSAFE)   != 0U;

    s_data.last_frame_ms = HAL_GetTick();
    s_data.frame_count++;

    return true;
}

/* ══════════════════════════════════════════════════════════════════════
 *  Public accessors
 * ══════════════════════════════════════════════════════════════════════ */

const sbus_data_t *SBUS_GetData(void)
{
    return &s_data;
}

bool SBUS_IsValid(void)
{
    if (s_data.failsafe) return false;
    uint32_t age_ms = HAL_GetTick() - s_data.last_frame_ms;
    return (age_ms < SBUS_FAILSAFE_TIMEOUT_MS);
}

/* ══════════════════════════════════════════════════════════════════════
 *  SBUS_ToMicros
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Linear map: [172, 1811] -> [1000, 2000] us
 *
 *   range_in  = 1811 - 172  = 1639
 *   range_out = 2000 - 1000 = 1000
 *
 *   us = 1000 + (raw - 172) * 1000 / 1639
 *
 * Integer arithmetic: multiply first to avoid precision loss.
 *   raw=172  -> us=1000   (zero throttle)
 *   raw=1024 -> us=1519   (centre, within ESC deadband of 1500)
 *   raw=1811 -> us=2000   (full throttle)
 */
uint16_t SBUS_ToMicros(uint16_t raw)
{
    if (raw < SBUS_CH_MIN) raw = SBUS_CH_MIN;
    if (raw > SBUS_CH_MAX) raw = SBUS_CH_MAX;

    return (uint16_t)(SBUS_US_MIN +
           ((uint32_t)(raw - SBUS_CH_MIN) * (SBUS_US_MAX - SBUS_US_MIN))
           / (SBUS_CH_MAX - SBUS_CH_MIN));
}

void SBUS_AllToMicros(uint16_t out[SBUS_CHANNELS])
{
    for (uint8_t i = 0U; i < SBUS_CHANNELS; i++)
    {
        out[i] = SBUS_ToMicros(s_data.channel[i]);
    }
}

/* ══════════════════════════════════════════════════════════════════════
 *  ISR handlers — wire into stm32f4xx_it.c
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * USART1 global IRQ — handles the IDLE-line event.
 *
 * When IDLE flag fires, a complete SBUS burst just ended.
 * We compute the DMA write pointer via NDTR and scan backwards
 * for the SBUS start byte to extract the last complete frame.
 */
void SBUS_USART_IRQHandler(void)
{
    if (__HAL_UART_GET_FLAG(&s_huart1, UART_FLAG_IDLE))
    {
        /* Clear IDLE flag on F4: read SR then DR */
        volatile uint32_t tmp;
        tmp = s_huart1.Instance->SR;
        tmp = s_huart1.Instance->DR;
        (void)tmp;

        /* Write pointer = buf_len - NDTR (DMA counts down) */
        uint16_t ndtr      = (uint16_t)__HAL_DMA_GET_COUNTER(&s_hdma_rx);
        uint16_t write_pos = SBUS_DMA_BUF_LEN - ndtr;

        /* Scan backwards for start byte; need at least 25 bytes before write_pos */
        for (int16_t i = (int16_t)write_pos - (int16_t)SBUS_FRAME_LEN;
             i >= 0; i--)
        {
            if (s_dma_buf[i] == SBUS_START_BYTE)
            {
                memcpy(s_rx_frame, &s_dma_buf[i], SBUS_FRAME_LEN);
                s_frame_ready = true;
                break;
            }
        }
    }

    HAL_UART_IRQHandler(&s_huart1);
}

void SBUS_DMA_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&s_hdma_rx);
}

/* ══════════════════════════════════════════════════════════════════════
 *  Private: channel bit unpacking
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Unpack 16 x 11-bit channels from the 22-byte SBUS payload.
 *
 * SBUS packs channels LSB-first into a flat bit stream.
 * Channel n occupies bits [n*11 ... n*11+10].
 *
 *   bit_pos   = n * 11
 *   byte_idx  = bit_pos / 8
 *   bit_shift = bit_pos % 8
 *   word = payload[byte_idx]
 *        | payload[byte_idx+1] << 8
 *        | payload[byte_idx+2] << 16
 *   channel_n = (word >> bit_shift) & 0x07FF
 *
 * The 3-byte window ensures all 11 bits are captured even when
 * the channel straddles 3 bytes (max bit_shift = 7: bits 7..17).
 */
static void sbus_unpack_channels(const uint8_t *payload)
{
    for (uint8_t ch = 0U; ch < SBUS_CHANNELS; ch++)
    {
        uint16_t bit_pos   = (uint16_t)(ch * 11U);
        uint16_t byte_idx  = bit_pos / 8U;
        uint8_t  bit_shift = (uint8_t)(bit_pos % 8U);

        uint32_t word = (uint32_t) payload[byte_idx]
                      | ((uint32_t)payload[byte_idx + 1U] << 8U)
                      | ((uint32_t)payload[byte_idx + 2U] << 16U);

        s_data.channel[ch] = (uint16_t)((word >> bit_shift) & 0x07FFU);
    }
}

/* ══════════════════════════════════════════════════════════════════════
 *  Private: peripheral init
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * PA10 -> AF7 (USART1_RX), Push-Pull, no pull, high speed.
 * TX not used — SBUS is receive-only.
 */
static void sbus_gpio_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin       = SBUS_RX_PIN;         /* PA10 */
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_NOPULL;
    gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
    gpio.Alternate = GPIO_AF7_USART1;

    HAL_GPIO_Init(SBUS_RX_PORT, &gpio);   /* GPIOA */
}

/**
 * DMA2 Stream5 Channel4 — USART1_RX circular mode.
 *
 * STM32F405 DMA mapping (RM0090 Table 42):
 *   USART1_RX -> DMA2, Stream5, Channel4
 */
static void sbus_dma_init(void)
{
    __HAL_RCC_DMA2_CLK_ENABLE();

    s_hdma_rx.Instance                 = DMA2_Stream5;
    s_hdma_rx.Init.Channel             = DMA_CHANNEL_4;
    s_hdma_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    s_hdma_rx.Init.PeriphInc           = DMA_PINC_DISABLE;   /* USART DR is fixed  */
    s_hdma_rx.Init.MemInc              = DMA_MINC_ENABLE;    /* advance in buffer  */
    s_hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    s_hdma_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    s_hdma_rx.Init.Mode                = DMA_CIRCULAR;        /* never stops        */
    s_hdma_rx.Init.Priority            = DMA_PRIORITY_HIGH;
    s_hdma_rx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;

    HAL_DMA_Init(&s_hdma_rx);

    /* Link DMA handle to UART handle so HAL_UART_Receive_DMA works */
    __HAL_LINKDMA(&s_huart1, hdmarx, s_hdma_rx);

    HAL_NVIC_SetPriority(DMA2_Stream5_IRQn, 5U, 0U);
    HAL_NVIC_EnableIRQ(DMA2_Stream5_IRQn);
}

/**
 * USART1 — 100,000 baud, 8 data bits, Even parity, 2 stop bits.
 *
 * IMPORTANT — word length with parity on STM32:
 *   Parity bit occupies the MSB of the USART word.
 *   8 data bits + 1 parity bit = 9 bits total ->
 *   WordLength must be UART_WORDLENGTH_9B.
 *   (Using 8B + parity would give only 7 data bits.)
 */
static void sbus_uart_init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();

    s_huart1.Instance          = USART1;
    s_huart1.Init.BaudRate     = SBUS_BAUD;               /* 100,000 bps        */
    s_huart1.Init.WordLength   = UART_WORDLENGTH_9B;      /* 8 data + 1 parity  */
    s_huart1.Init.StopBits     = UART_STOPBITS_2;         /* 2 stop bits        */
    s_huart1.Init.Parity       = UART_PARITY_EVEN;        /* Even parity        */
    s_huart1.Init.Mode         = UART_MODE_RX;            /* RX only            */
    s_huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    s_huart1.Init.OverSampling = UART_OVERSAMPLING_16;

    HAL_UART_Init(&s_huart1);

    /* USART1 global IRQ for IDLE-line detection */
    HAL_NVIC_SetPriority(USART1_IRQn, 5U, 0U);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
}


