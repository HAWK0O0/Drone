/**
 * @file    pin_config.h
 * @brief   Hardware pin mapping for STM32F405RGT6 Flight Controller
 *          Board: WeAct-STM32F4 Core Board V1.1
 */

#ifndef FC_PIN_CONFIG_H
#define FC_PIN_CONFIG_H

#include "stm32f4xx_hal.h"

/* ------------------------------------------------------------------ */
/*  [1] Motors — TIM3 CH1-CH4 (PWM)                                   */
/* ------------------------------------------------------------------ */
#define MOTOR_1_PIN         GPIO_PIN_6    /* PA6  — TIM3 CH1 */
#define MOTOR_1_PORT        GPIOA
#define MOTOR_2_PIN         GPIO_PIN_7    /* PA7  — TIM3 CH2 */
#define MOTOR_2_PORT        GPIOA
#define MOTOR_3_PIN         GPIO_PIN_0    /* PB0  — TIM3 CH3 */
#define MOTOR_3_PORT        GPIOB
#define MOTOR_4_PIN         GPIO_PIN_1    /* PB1  — TIM3 CH4 */
#define MOTOR_4_PORT        GPIOB

/* ------------------------------------------------------------------ */
/*  [2] I2C1 — IMU (MPU6050 / ICM42688)                               */
/* ------------------------------------------------------------------ */
#define I2C1_SCL_PIN        GPIO_PIN_6    /* PB6 */
#define I2C1_SCL_PORT       GPIOB
#define I2C1_SDA_PIN        GPIO_PIN_7    /* PB7 */
#define I2C1_SDA_PORT       GPIOB

/* ------------------------------------------------------------------ */
/*  [3] I2C2 — Barometer / Magnetometer                               */
/* ------------------------------------------------------------------ */
#define I2C2_SCL_PIN        GPIO_PIN_10   /* PB10 */
#define I2C2_SCL_PORT       GPIOB
#define I2C2_SDA_PIN        GPIO_PIN_11   /* PB11 */
#define I2C2_SDA_PORT       GPIOB

/* ------------------------------------------------------------------ */
/*  [4] USART1 — SBUS RC Receiver (inverted, PA10)                    */
/* ------------------------------------------------------------------ */
#define SBUS_RX_PIN         GPIO_PIN_10   /* PA10 — USART1_RX */
#define SBUS_RX_PORT        GPIOA

/* ------------------------------------------------------------------ */
/*  [5] USART6 — GPS                                                   */
/* ------------------------------------------------------------------ */
#define GPS_RX_PIN          GPIO_PIN_6    /* PC6 — USART6_RX */
#define GPS_RX_PORT         GPIOC
#define GPS_TX_PIN          GPIO_PIN_7    /* PC7 — USART6_TX */
#define GPS_TX_PORT         GPIOC

/* ------------------------------------------------------------------ */
/*  [6] USART2 — ESP32-S3CAM Telemetry                                */
/* ------------------------------------------------------------------ */
#define CAM_TX_PIN          GPIO_PIN_2    /* PA2 — USART2_TX */
#define CAM_TX_PORT         GPIOA
#define CAM_RX_PIN          GPIO_PIN_3    /* PA3 — USART2_RX */
#define CAM_RX_PORT         GPIOA

/* ------------------------------------------------------------------ */
/*  [7] SDIO — TF Card (4-bit mode)                                   */
/* ------------------------------------------------------------------ */
#define SDIO_D0_PIN         GPIO_PIN_8    /* PC8  */
#define SDIO_D1_PIN         GPIO_PIN_9    /* PC9  */
#define SDIO_D2_PIN         GPIO_PIN_10   /* PC10 */
#define SDIO_D3_PIN         GPIO_PIN_11   /* PC11 */
#define SDIO_CLK_PIN        GPIO_PIN_12   /* PC12 */
#define SDIO_PORT           GPIOC
#define SDIO_CMD_PIN        GPIO_PIN_2    /* PD2  */
#define SDIO_CMD_PORT       GPIOD
#define SDIO_DETECT_PIN     GPIO_PIN_8    /* PA8  — Card Detect */
#define SDIO_DETECT_PORT    GPIOA

/* ------------------------------------------------------------------ */
/*  [8] Peripherals — Buzzer & LEDs                                    */
/* ------------------------------------------------------------------ */
#define BUZZER_PIN          GPIO_PIN_14   /* PB14 — TIM12 CH1 */
#define BUZZER_PORT         GPIOB
#define LED_BLUE_PIN        GPIO_PIN_12   /* PB12 */
#define LED_BLUE_PORT       GPIOB
#define LED_RED_PIN         GPIO_PIN_13   /* PB13 */
#define LED_RED_PORT        GPIOB

/* ------------------------------------------------------------------ */
/*  [9] On-Board Peripherals                                           */
/* ------------------------------------------------------------------ */
#define BOARD_LED_PIN       GPIO_PIN_2    /* PB2  — on-board blue LED  */
#define BOARD_LED_PORT      GPIOB
#define BOARD_KEY_PIN       GPIO_PIN_0    /* PA0  — user button (WKUP) */
#define BOARD_KEY_PORT      GPIOA

/* ------------------------------------------------------------------ */
/*  [10] USB OTG FS                                                    */
/* ------------------------------------------------------------------ */
#define USB_DM_PIN          GPIO_PIN_11   /* PA11 */
#define USB_DP_PIN          GPIO_PIN_12   /* PA12 */
#define USB_PORT            GPIOA

/* ------------------------------------------------------------------ */
/*  [11] Available / Expansion Pins                                    */
/* ------------------------------------------------------------------ */
/* PA4, PA5  — SPI1 or ADC                                            */
/* PB4, PB5  — SPI1 or extra PWM                                      */
/* PB8, PB9  — CAN Bus or backup I2C                                  */
/* PC0–PC5   — ADC (battery voltage monitoring 11.1V)                 */

#endif /* FC_PIN_CONFIG_H */
