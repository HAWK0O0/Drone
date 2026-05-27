/**
 * @file    telemetry.h
 * @brief   MAVLink v2 telemetry over USART2 (PA2 TX, PA3 RX) to ESP32-S3CAM
 */

#ifndef FC_TELEMETRY_H
#define FC_TELEMETRY_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

/** Initialise USART2 at 115200 baud for MAVLink */
void Telemetry_Init(void);

/** Send periodic MAVLink messages — call at TELEMETRY_RATE_HZ (10 Hz) */
void Telemetry_Send(void);

/** Process received MAVLink bytes from DMA buffer */
void Telemetry_Receive(void);

/** Send HEARTBEAT message */
void Telemetry_SendHeartbeat(void);

/** Send ATTITUDE (roll/pitch/yaw) */
void Telemetry_SendAttitude(void);

/** Send GPS_RAW_INT */
void Telemetry_SendGPS(void);

/** Send RC_CHANNELS */
void Telemetry_SendRC(void);

/** Send SYS_STATUS (battery voltage, load) */
void Telemetry_SendSysStatus(void);

#endif /* FC_TELEMETRY_H */
