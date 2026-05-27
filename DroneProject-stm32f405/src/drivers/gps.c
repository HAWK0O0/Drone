/**
 * @file    gps.c
 * @brief   GPS NMEA parser — USART6 / PC6 RX / PC7 TX / DMA2-Stream1
 *
 * ── NMEA sentence structure ──────────────────────────────────────────
 *   $TTSSS,f1,f2,...,fN*CS\r\n
 *   TT  = talker ID (GP=GPS, GN=GNSS multi-constellation)
 *   SSS = sentence type (GGA, RMC, ...)
 *   CS  = 2-hex checksum = XOR of bytes between $ and * (exclusive)
 *
 * ── GPGGA / GNGGA fields ────────────────────────────────────────────
 *   0 HHMMSS.ss   UTC time
 *   1 DDMM.mmmm   Latitude
 *   2 N/S
 *   3 DDDMM.mmmm  Longitude
 *   4 E/W
 *   5 0-2         Fix quality (0=none, 1=GPS, 2=DGPS)
 *   6 NN          Satellites used
 *   7 H.H         HDOP
 *   8 A.A         Altitude MSL
 *   9 M
 *
 * ── GPRMC / GNRMC fields ────────────────────────────────────────────
 *   0 HHMMSS.ss   UTC time
 *   1 A/V         Status (A=active, V=void)
 *   2 DDMM.mmmm   Latitude
 *   3 N/S
 *   4 DDDMM.mmmm  Longitude
 *   5 E/W
 *   6 S.SS         Speed over ground (knots)
 *   7 D.DD         Course over ground (degrees true)
 *   8 DDMMYY       Date
 *
 * ── Coordinate format ────────────────────────────────────────────────
 *   DDMM.MMMM  →  decimal degrees = DD + MM.MMMM/60
 *   e.g. "5321.5802" N  →  53 + 21.5802/60 = 53.35967°N
 *
 * ── DMA circular buffer processing ──────────────────────────────────
 *   s_last_pos tracks where we last read in the 256-byte circular buffer.
 *   GPS_Process() computes current write position (DMA_BUF_LEN - NDTR)
 *   and walks bytes from s_last_pos to write_pos, feeding them into the
 *   sentence accumulator.  On '\n', a complete sentence is dispatched.
 */

#include "drivers/gps.h"
#include "pin_config.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ── Private constants ───────────────────────────────────────────────*/
#define GPS_BAUD            9600U
#define KNOTS_TO_MPS        0.51444f   /* 1 knot = 0.51444 m/s */

/* ── Private state ───────────────────────────────────────────────────*/
static UART_HandleTypeDef  s_huart6;
static DMA_HandleTypeDef   s_hdma_rx;

static uint8_t  s_dma_buf[GPS_DMA_BUF_LEN];  /* circular DMA buffer */
static uint16_t s_last_pos = 0U;              /* last processed byte */

/* Sentence accumulator — filled byte-by-byte in GPS_Process() */
static char    s_sentence[GPS_SENTENCE_MAX_LEN];
static uint8_t s_sentence_len = 0U;

static gps_data_t s_gps = {0};

/* ── Forward declarations ────────────────────────────────────────────*/
static void    gps_gpio_init(void);
static void    gps_dma_init(void);
static void    gps_uart_init(void);
static bool    nmea_checksum_valid(const char *sentence);
static void    nmea_dispatch(const char *sentence);
static void    parse_gpgga(const char *sentence);
static void    parse_gprmc(const char *sentence);
static double  nmea_parse_coord(const char *str, char hemi);
static void    nmea_parse_time(const char *str,
                               uint8_t *hour, uint8_t *min, uint8_t *sec);

/* ══════════════════════════════════════════════════════════════════════
 *  GPS_Init
 * ══════════════════════════════════════════════════════════════════════ */
void GPS_Init(void)
{
    gps_gpio_init();
    gps_dma_init();
    gps_uart_init();

    __HAL_UART_ENABLE_IT(&s_huart6, UART_IT_IDLE);
    HAL_UART_Receive_DMA(&s_huart6, s_dma_buf, GPS_DMA_BUF_LEN);
}

/* ══════════════════════════════════════════════════════════════════════
 *  GPS_Process  (call from main loop / ≤10 Hz task)
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Walk the circular DMA buffer from where we last left off to the current
 * DMA write position, feeding each byte into the sentence accumulator.
 *
 * The accumulator starts on '$' and terminates on '\n'.  When a sentence
 * is complete it is validated (checksum) and dispatched to the appropriate
 * parser.
 */
void GPS_Process(void)
{
    uint16_t write_pos =
        (uint16_t)(GPS_DMA_BUF_LEN - __HAL_DMA_GET_COUNTER(&s_hdma_rx));

    while (s_last_pos != write_pos)
    {
        uint8_t byte = s_dma_buf[s_last_pos];
        s_last_pos = (uint16_t)((s_last_pos + 1U) % GPS_DMA_BUF_LEN);

        if (byte == '$')
        {
            /* Start of new sentence — reset accumulator */
            s_sentence_len = 0U;
            s_sentence[s_sentence_len++] = (char)byte;
        }
        else if (byte == '\n')
        {
            if (s_sentence_len > 0U)
            {
                s_sentence[s_sentence_len] = '\0';
                if (nmea_checksum_valid(s_sentence))
                {
                    nmea_dispatch(s_sentence);
                }
                s_sentence_len = 0U;
            }
        }
        else if (byte != '\r')
        {
            /* Accumulate (skip bare \r) */
            if (s_sentence_len > 0U &&
                s_sentence_len < GPS_SENTENCE_MAX_LEN - 1U)
            {
                s_sentence[s_sentence_len++] = (char)byte;
            }
        }
    }
}

/* ══════════════════════════════════════════════════════════════════════
 *  Public accessors
 * ══════════════════════════════════════════════════════════════════════ */

const gps_data_t *GPS_GetData(void)
{
    return &s_gps;
}

bool GPS_IsValid(void)
{
    return (s_gps.fix > 0U) &&
           ((HAL_GetTick() - s_gps.last_fix_ms) < GPS_FIX_TIMEOUT_MS);
}

/* ══════════════════════════════════════════════════════════════════════
 *  ISR handlers — wire into stm32f4xx_it.c
 * ══════════════════════════════════════════════════════════════════════ */

void GPS_USART_IRQHandler(void)
{
    if (__HAL_UART_GET_FLAG(&s_huart6, UART_FLAG_IDLE))
    {
        /* Clear IDLE flag on F4: read SR then DR */
        volatile uint32_t tmp;
        tmp = s_huart6.Instance->SR;
        tmp = s_huart6.Instance->DR;
        (void)tmp;
    }
    HAL_UART_IRQHandler(&s_huart6);
}

void GPS_DMA_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&s_hdma_rx);
}

/* ══════════════════════════════════════════════════════════════════════
 *  Private: NMEA checksum validation
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Validate the NMEA checksum.
 *
 * Expected format: $...*HH  where HH = XOR of bytes between $ and *.
 *
 * @return true  Checksum matches.
 * @return false Sentence has no '*', or checksum mismatch.
 */
static bool nmea_checksum_valid(const char *sentence)
{
    /* Find the '*' delimiter */
    const char *star = strchr(sentence, '*');
    if (star == NULL) return false;

    /* Compute XOR from char after '$' up to but not including '*' */
    uint8_t computed = 0U;
    for (const char *p = sentence + 1; p != star; p++)
    {
        computed ^= (uint8_t)*p;
    }

    /* Parse the two-char hex checksum after '*' */
    uint8_t received = (uint8_t)strtol(star + 1, NULL, 16);
    return (computed == received);
}

/* ── Dispatch to the correct parser ──────────────────────────────── */
static void nmea_dispatch(const char *sentence)
{
    /* Accept both GP (pure GPS) and GN (multi-constellation GNSS) talkers */
    if (strncmp(sentence + 1, "GPGGA", 5) == 0 ||
        strncmp(sentence + 1, "GNGGA", 5) == 0)
    {
        parse_gpgga(sentence);
    }
    else if (strncmp(sentence + 1, "GPRMC", 5) == 0 ||
             strncmp(sentence + 1, "GNRMC", 5) == 0)
    {
        parse_gprmc(sentence);
    }
}

/* ══════════════════════════════════════════════════════════════════════
 *  Private: GPGGA parser
 *
 *  $GPGGA,HHMMSS.ss,DDMM.mmmm,N,DDDMM.mmmm,E,Q,SS,H.H,ALT,M,...*CS
 *  Field:  0        1          2 3           4 5 6  7   8   9
 * ══════════════════════════════════════════════════════════════════════ */
static void parse_gpgga(const char *sentence)
{
    /* Tokenise a local copy so we don't modify s_sentence */
    char buf[GPS_SENTENCE_MAX_LEN];
    strncpy(buf, sentence, GPS_SENTENCE_MAX_LEN - 1U);
    buf[GPS_SENTENCE_MAX_LEN - 1U] = '\0';

    /* Walk fields by comma */
    char *fields[15] = {0};
    uint8_t field_count = 0U;
    char *tok = strtok(buf, ",");
    while (tok != NULL && field_count < 15U)
    {
        fields[field_count++] = tok;
        tok = strtok(NULL, ",*");
    }
    if (field_count < 10U) return;

    /* Field 5: fix quality */
    uint8_t fix = (uint8_t)atoi(fields[5]);
    if (fix == GPS_FIX_NONE) return;  /* no fix — don't update position */

    /* Field 0: UTC time HHMMSS.ss */
    nmea_parse_time(fields[0], &s_gps.hour, &s_gps.minute, &s_gps.second);

    /* Fields 1-2: latitude */
    if (fields[1][0] != '\0' && fields[2][0] != '\0')
    {
        s_gps.latitude = nmea_parse_coord(fields[1], fields[2][0]);
    }

    /* Fields 3-4: longitude */
    if (fields[3][0] != '\0' && fields[4][0] != '\0')
    {
        s_gps.longitude = nmea_parse_coord(fields[3], fields[4][0]);
    }

    /* Field 6: satellites */
    s_gps.satellites = (uint8_t)atoi(fields[6]);

    /* Field 7: HDOP */
    s_gps.hdop = strtof(fields[7], NULL);

    /* Field 8: altitude MSL */
    s_gps.altitude_m = strtof(fields[8], NULL);

    s_gps.fix           = fix;
    s_gps.last_fix_ms   = HAL_GetTick();
    s_gps.frame_count++;
}

/* ══════════════════════════════════════════════════════════════════════
 *  Private: GPRMC parser
 *
 *  $GPRMC,HHMMSS.ss,A,DDMM.mmmm,N,DDDMM.mmmm,E,SPD,CRS,DDMMYY,...*CS
 *  Field:  0        1 2          3 4           5 6   7   8
 * ══════════════════════════════════════════════════════════════════════ */
static void parse_gprmc(const char *sentence)
{
    char buf[GPS_SENTENCE_MAX_LEN];
    strncpy(buf, sentence, GPS_SENTENCE_MAX_LEN - 1U);
    buf[GPS_SENTENCE_MAX_LEN - 1U] = '\0';

    char *fields[12] = {0};
    uint8_t field_count = 0U;
    char *tok = strtok(buf, ",");
    while (tok != NULL && field_count < 12U)
    {
        fields[field_count++] = tok;
        tok = strtok(NULL, ",*");
    }
    if (field_count < 8U) return;

    /* Field 1: status — A=active, V=void */
    if (fields[1][0] != 'A') return;

    /* Field 0: UTC time */
    nmea_parse_time(fields[0], &s_gps.hour, &s_gps.minute, &s_gps.second);

    /* Fields 2-3: latitude */
    if (fields[2][0] != '\0' && fields[3][0] != '\0')
    {
        s_gps.latitude = nmea_parse_coord(fields[2], fields[3][0]);
    }

    /* Fields 4-5: longitude */
    if (fields[4][0] != '\0' && fields[5][0] != '\0')
    {
        s_gps.longitude = nmea_parse_coord(fields[4], fields[5][0]);
    }

    /* Field 6: speed in knots → m/s */
    if (fields[6][0] != '\0')
    {
        s_gps.speed_mps = strtof(fields[6], NULL) * KNOTS_TO_MPS;
    }

    /* Field 7: true course */
    if (fields[7][0] != '\0')
    {
        s_gps.course_deg = strtof(fields[7], NULL);
    }
}

/* ══════════════════════════════════════════════════════════════════════
 *  Private: coordinate conversion
 *
 *  NMEA format: DDDMM.MMMM
 *    degrees = int(raw / 100)
 *    minutes = raw - degrees * 100
 *    decimal = degrees + minutes / 60
 * ══════════════════════════════════════════════════════════════════════ */
static double nmea_parse_coord(const char *str, char hemi)
{
    if (str == NULL || str[0] == '\0') return 0.0;

    double raw     = strtod(str, NULL);
    int    degrees = (int)(raw / 100.0);
    double minutes = raw - (double)(degrees * 100);
    double result  = (double)degrees + minutes / 60.0;

    if (hemi == 'S' || hemi == 'W') result = -result;
    return result;
}

/* ── Parse HHMMSS.ss time field ───────────────────────────────────── */
static void nmea_parse_time(const char *str,
                             uint8_t *hour, uint8_t *min, uint8_t *sec)
{
    if (str == NULL || str[0] == '\0') return;

    /* HHMMSS.ss — parse as integer, then split */
    long t     = atol(str);          /* e.g. 123519 from "123519.00" */
    *hour      = (uint8_t)(t / 10000L);
    *min       = (uint8_t)((t / 100L) % 100L);
    *sec       = (uint8_t)(t % 100L);
}

/* ══════════════════════════════════════════════════════════════════════
 *  Private: peripheral init
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * PC6 → AF8 USART6_RX  (input, push-pull, no pull)
 * PC7 → AF8 USART6_TX  (output, push-pull, no pull)
 */
static void gps_gpio_init(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin       = GPS_RX_PIN | GPS_TX_PIN;  /* PC6, PC7 */
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_NOPULL;
    gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
    gpio.Alternate = GPIO_AF8_USART6;

    HAL_GPIO_Init(GPIOC, &gpio);
}

/**
 * DMA2 Stream1 Channel5 — USART6_RX circular.
 *
 * STM32F405 DMA mapping (RM0090 Table 42):
 *   USART6_RX → DMA2_Stream1_CH5  (primary)
 *               DMA2_Stream2_CH5  (alternate — not used here)
 */
static void gps_dma_init(void)
{
    __HAL_RCC_DMA2_CLK_ENABLE();

    s_hdma_rx.Instance                 = DMA2_Stream1;
    s_hdma_rx.Init.Channel             = DMA_CHANNEL_5;
    s_hdma_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    s_hdma_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
    s_hdma_rx.Init.MemInc              = DMA_MINC_ENABLE;
    s_hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    s_hdma_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    s_hdma_rx.Init.Mode                = DMA_CIRCULAR;
    s_hdma_rx.Init.Priority            = DMA_PRIORITY_MEDIUM;
    s_hdma_rx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;

    HAL_DMA_Init(&s_hdma_rx);
    __HAL_LINKDMA(&s_huart6, hdmarx, s_hdma_rx);

    HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, 6U, 0U);
    HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);
}

/**
 * USART6 — 9600 baud, 8N1.
 * GPS modules default to 9600 baud.  No parity, 1 stop bit.
 */
static void gps_uart_init(void)
{
    __HAL_RCC_USART6_CLK_ENABLE();

    s_huart6.Instance          = USART6;
    s_huart6.Init.BaudRate     = GPS_BAUD;
    s_huart6.Init.WordLength   = UART_WORDLENGTH_8B;
    s_huart6.Init.StopBits     = UART_STOPBITS_1;
    s_huart6.Init.Parity       = UART_PARITY_NONE;
    s_huart6.Init.Mode         = UART_MODE_RX;     /* RX only for parsing */
    s_huart6.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    s_huart6.Init.OverSampling = UART_OVERSAMPLING_16;

    HAL_UART_Init(&s_huart6);

    HAL_NVIC_SetPriority(USART6_IRQn, 6U, 0U);
    HAL_NVIC_EnableIRQ(USART6_IRQn);
}
