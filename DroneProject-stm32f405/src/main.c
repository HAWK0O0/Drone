/**
 * @file    main.c
 * @brief   Flight Controller — Full Integration & Main Loop
 *          STM32F405RGT6 @ 168 MHz  |  X-frame quadcopter
 *
 * ── Loop Architecture ────────────────────────────────────────────────
 *
 *  [Every 1 ms — 1 kHz]   — timed by DWT cycle counter (168 000 cy/tick)
 *    Safety_Refresh()         kick IWDG watchdog
 *    SBUS_Parse()             decode any buffered RC frame
 *    AHRS_Update()            Madgwick 6-DOF filter → quaternion → Euler
 *    Attitude_Update()        cascaded PID (angle→rate) → Motor_SetAll()
 *
 *  [Every 10 ms — 100 Hz]
 *    Safety_Update()          arm-switch poll, failsafe detect, buzzer events
 *    LED_Update()             blink state machines
 *    Buzzer_Update()          tone pattern state machine
 *    LED pattern sync         reflect arm state onto LED colours
 *
 *  [Every 100 ms — 10 Hz]
 *    GPS_Process()            parse buffered NMEA sentences
 *    USB Serial telemetry     printf → CDC → host serial monitor
 *
 *  [Every 1 s — 1 Hz]
 *    Full status line         GPS coords + frame counters
 *
 * ── Pin Summary ──────────────────────────────────────────────────────
 *  TIM3  PA6/PA7/PB0/PB1 → M1–M4 PWM 400 Hz
 *  I2C1  PB6/PB7          → MPU6050 @ 400 kHz
 *  USART1 PA10            → SBUS 100 k/8E2 inv.
 *  USART6 PC6/PC7         → GPS 9600/8N1
 *  TIM12  PB14            → Buzzer 2.8 kHz PWM
 *  USB OTG-FS PA11/PA12   → Virtual COM Port (printf)
 *  PB12 Blue LED  PB13 Red LED
 */

#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "pin_config.h"
#include "config.h"
#include "system_clock.h"
#include "scheduler.h"
#include "safety.h"

/* Drivers */
#include "drivers/motor.h"
#include "drivers/imu.h"
#include "drivers/sbus.h"
#include "drivers/gps.h"
#include "drivers/buzzer.h"
#include "drivers/led.h"
#include "drivers/sdcard.h"

/* AHRS */
#include "ahrs/ahrs.h"

/* Control */
#include "control/attitude.h"

/* USB CDC — printf over Type-C */
#include "usb/usb_device.h"
#include "usb/usbd_cdc_if.h"

/* SD Blackbox + ESP32 Telemetry */
#include "blackbox.h"
#include "telemetry/telemetry.h"

/* ------------------------------------------------------------------ */
/*  Private helpers                                                     */
/* ------------------------------------------------------------------ */

/** Update LED patterns to match current arm state. */
static void sync_led_to_arm_state(void)
{
    switch (Safety_GetState())
    {
        case ARM_STATE_ARMED:
            LED_SetPattern(LED_BLUE, LED_PATTERN_ON);
            LED_SetPattern(LED_RED,  LED_PATTERN_OFF);
            break;

        case ARM_STATE_FAILSAFE:
        case ARM_STATE_ERROR:
            LED_SetPattern(LED_BLUE, LED_PATTERN_OFF);
            LED_SetPattern(LED_RED,  LED_PATTERN_BLINK_FAST);
            break;

        case ARM_STATE_PRE_ARM_CHECK:
            LED_SetPattern(LED_BLUE, LED_PATTERN_BLINK_FAST);
            LED_SetPattern(LED_RED,  LED_PATTERN_OFF);
            break;

        default: /* DISARMED */
            LED_SetPattern(LED_BLUE, LED_PATTERN_BLINK_SLOW);
            LED_SetPattern(LED_RED,  LED_PATTERN_OFF);
            break;
    }
}

/** Print compact attitude + state telemetry line (10 Hz).
 *  Always prints AHRS angles and raw gyro — safe to use even in FAILSAFE / RC:LOST. */
static void print_telemetry_10hz(void)
{
    const ahrs_state_t *att = AHRS_GetState();
    const char *arm_str;

    switch (Safety_GetState())
    {
        case ARM_STATE_ARMED:         arm_str = "ARMED   "; break;
        case ARM_STATE_FAILSAFE:      arm_str = "FAILSAFE"; break;
        case ARM_STATE_PRE_ARM_CHECK: arm_str = "PRE-ARM "; break;
        case ARM_STATE_ERROR:         arm_str = "ERROR   "; break;
        default:                      arm_str = "DISARMED"; break;
    }

    /* AHRS angles — always valid regardless of RC state */
    printf("[%5lu ms] %s | R:%+6.1f P:%+6.1f Y:%6.1f",
           (unsigned long)HAL_GetTick(),
           arm_str,
           (double)att->roll,
           (double)att->pitch,
           (double)att->yaw);

    if (SBUS_IsValid())
    {
        const sbus_data_t *rc = SBUS_GetData();
        uint16_t thr_us = SBUS_ToMicros(rc->channel[SBUS_CH_THROTTLE]);
        printf(" | THR:%4u RC:OK\r\n", (unsigned)thr_us);
    }
    else
    {
        /* RC lost — print raw gyro so sensors can be verified without radio */
        imu_data_t imu = {0};
        (void)IMU_Read(&imu);
        printf(" | RC:LOST | Gyro[dps] X:%+6.1f Y:%+6.1f Z:%+6.1f\r\n",
               (double)imu.gyro_x,
               (double)imu.gyro_y,
               (double)imu.gyro_z);
    }
}

/** Print full 1 Hz status (GPS + counters). */
static void print_status_1hz(void)
{
    const gps_data_t *gps = GPS_GetData();

    if (GPS_IsValid())
    {
        printf("[1Hz] GPS:3D(%usv) lat=%.6f lon=%.6f alt=%.1fm spd=%.1fm/s\r\n",
               (unsigned)gps->satellites,
               gps->latitude,
               gps->longitude,
               (double)gps->altitude_m,
               (double)gps->speed_mps);
    }
    else
    {
        printf("[1Hz] GPS:NO-FIX | AHRS:%s\r\n",
               AHRS_IsConverged() ? "CONV" : "INIT");
    }
}

/* ------------------------------------------------------------------ */
/*  Error handler                                                       */
/* ------------------------------------------------------------------ */

void Error_Handler(void)
{
    __disable_irq();

    /* Best-effort motor stop — direct register write, no HAL needed */
    if (TIM3->CR1 & TIM_CR1_CEN)
    {
        TIM3->CCR1 = 1000U;
        TIM3->CCR2 = 1000U;
        TIM3->CCR3 = 1000U;
        TIM3->CCR4 = 1000U;
    }

    /* Blink board LED rapidly to signal fault */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIOB->MODER |= (1U << (2 * 2));  /* PB2 = output */
    while (1)
    {
        GPIOB->ODR ^= GPIO_PIN_2;
        for (volatile uint32_t i = 0; i < 200000UL; i++) {}
    }
}

/* ------------------------------------------------------------------ */
/*  Main                                                                */
/* ------------------------------------------------------------------ */

int main(void)
{
    /* ── Core init ───────────────────────────────────────────────── */
    HAL_Init();
    SystemClock_Config();   /* FPU on, HSE+PLL → 168 MHz */

    /* ── Status output peripherals (init first so printf works early) */
    LED_Init();
    LED_SetPattern(LED_BLUE, LED_PATTERN_BLINK_FAST);   /* boot indicator */

    Buzzer_Init();
    Buzzer_Play(BUZZ_BOOT);

    USB_Device_Init();          /* Virtual COM Port — init before first printf */
    HAL_Delay(500U);            /* wait for USB enumeration on host            */

    printf("\r\n");
    printf("==============================================\r\n");
    printf(" FC-STM32F405  Flight Controller  Boot\r\n");
    printf(" SYSCLK: %lu MHz\r\n", (unsigned long)(SystemClock_GetFreqHz() / 1000000U));
    printf("==============================================\r\n");

    /* ── Motor hardware init ─────────────────────────────────────── */
    Motor_Init();
    printf("[init] Motor PWM ready (TIM3 @ 400 Hz)\r\n");

    /* ── IMU ─────────────────────────────────────────────────────── */
    printf("[init] IMU init... ");
    if (IMU_Init() != HAL_OK)
    {
        printf("FAIL\r\n");
        Error_Handler();
    }
    printf("OK\r\n");

    printf("[init] IMU calibrating (keep drone still)... ");
    (void)IMU_Calibrate();      /* ~500 ms blocking, drone must not move */
    printf("done\r\n");

    /* ── RC + GPS ────────────────────────────────────────────────── */
    SBUS_Init();
    printf("[init] SBUS receiver ready (USART1 @ 100kbaud)\r\n");

    GPS_Init();
    printf("[init] GPS ready (USART6 @ 9600)\r\n");

    /* ── AHRS + Safety + Scheduler ───────────────────────────────── */
    AHRS_Init();
    printf("[init] AHRS Madgwick filter ready\r\n");

    Safety_Init();
    printf("[init] Safety / IWDG ready\r\n");

    Scheduler_Init();           /* DWT cycle counter — must be after clock cfg */

    /* ── Attitude controller ─────────────────────────────────────── */
    static attitude_ctrl_t attitude;
    Attitude_Init(&attitude);
    printf("[init] Cascaded PID controller ready\r\n");

    /* ── SD Blackbox ─────────────────────────────────────────────── */
    if (SDCard_Init() == SDCARD_OK)
    {
        Blackbox_Init();
        printf("[init] SD card mounted — blackbox ready (opens on arm)\r\n");
    }
    else
    {
        printf("[init] SD Blackbox: no card — flying without log\r\n");
    }

    /* ── ESP32 Telemetry (MAVLink v1) ────────────────────────────── */
    Telemetry_Init();
    printf("[init] Telemetry USART2 ready (115200 baud, MAVLink v1)\r\n");

    /* ── Ready ───────────────────────────────────────────────────── */
    LED_SetPattern(LED_BLUE, LED_PATTERN_BLINK_SLOW);   /* waiting to arm */
    printf("[init] ** Boot complete — waiting for arm switch **\r\n\r\n");

    /* ── Main loop ───────────────────────────────────────────────── */
    arm_state_t prev_arm_state = ARM_STATE_DISARMED;

    while (1)
    {
        /* ── [1 kHz] Wait for next 1 ms tick boundary ── */
        Scheduler_WaitNextTick();

        /* IWDG watchdog kick — must happen every loop iteration */
        Safety_Refresh();

        /* ── [1 kHz] Flight-critical inner loop ── */
        SBUS_Parse();               /* decode any new 25-byte SBUS frame       */
        AHRS_Update();              /* Madgwick 6-DOF → quaternion → Euler     */
        Attitude_Update(&attitude); /* cascaded PID → Motor_SetAll()           */

        /* ── [100 Hz] Every 10 ms ── */
        if (Scheduler_ShouldRunTask(100U))
        {
            Safety_Update();        /* arm-switch poll, failsafe, buzzer events */
            LED_Update();           /* blink state machines                     */
            Buzzer_Update();        /* tone pattern state machine               */
            sync_led_to_arm_state();

            /* Detect arm state transitions → open/close blackbox file */
            arm_state_t curr_arm = Safety_GetState();
            if (prev_arm_state != ARM_STATE_ARMED && curr_arm == ARM_STATE_ARMED)
            {
                Blackbox_Open();
                printf("[BB] Arm → new log file opened\r\n");
            }
            else if (prev_arm_state == ARM_STATE_ARMED && curr_arm != ARM_STATE_ARMED)
            {
                Blackbox_Close();
                printf("[BB] Disarm → log file closed\r\n");
            }
            prev_arm_state = curr_arm;
        }

        /* ── [50 Hz] Every 20 ms — blackbox CSV row ── */
        if (Scheduler_ShouldRunTask(50U))
        {
            Blackbox_Log();         /* CSV row → SD card (only when armed)     */
        }

        /* ── [10 Hz] Every 100 ms ── */
        if (Scheduler_ShouldRunTask(10U))
        {
            GPS_Process();              /* parse buffered NMEA sentences */
            Telemetry_Send();           /* 5× MAVLink v1 → ESP32 USART2  */
            print_telemetry_10hz();     /* USB CDC: angles + arm state   */
        }

        /* ── [1 Hz] Every 1000 ms ── */
        if (Scheduler_ShouldRunTask(1U))
        {
            print_status_1hz();     /* USB CDC: GPS fix + AHRS convergence */
        }
    }
}
