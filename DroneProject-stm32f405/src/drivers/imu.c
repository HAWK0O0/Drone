/**
 * @file    imu.c
 * @brief   MPU6050 IMU driver via I2C1 (PB6=SCL, PB7=SDA) @ 400 kHz
 *
 * ── I2C clock derivation ─────────────────────────────────────────────
 *   APB1 = 42 MHz  (SYSCLK 168 ÷ APB1 prescaler 4)
 *   HAL_I2C_Init configures the CCR register automatically
 *   from ClockSpeed + DutyCycle at the given APB1 frequency.
 *
 * ── Data burst layout (14 bytes from reg 0x3B) ──────────────────────
 *   Byte  0-1  : ACCEL_XOUT  (big-endian int16)
 *   Byte  2-3  : ACCEL_YOUT
 *   Byte  4-5  : ACCEL_ZOUT
 *   Byte  6-7  : TEMP_OUT
 *   Byte  8-9  : GYRO_XOUT
 *   Byte 10-11 : GYRO_YOUT
 *   Byte 12-13 : GYRO_ZOUT
 *
 * ── Scaling ──────────────────────────────────────────────────────────
 *   Accel: raw_int16 / 4096.0  →  g  →  × 9.80665  →  m/s²
 *   Gyro : raw_int16 / 16.4    →  °/s
 *   Temp : raw_int16 / 340.0 + 36.53 → °C
 */

#include "drivers/imu.h"
#include "pin_config.h"

/* ── Private constants ───────────────────────────────────────────────*/
#define GRAVITY_MSS     9.80665f           /* standard gravity m/s²    */
#define BURST_LEN       14U                /* bytes in one sensor read  */

/* ── Private state ───────────────────────────────────────────────────*/
static I2C_HandleTypeDef s_hi2c1;
static imu_data_t        s_bias  = {0};   /* calibration offsets       */
static bool              s_ready = false; /* set true after IMU_Init() */

/* ── Private helpers ─────────────────────────────────────────────────*/
static HAL_StatusTypeDef imu_write_reg(uint8_t reg, uint8_t val);
static HAL_StatusTypeDef imu_read_regs(uint8_t reg, uint8_t *buf, uint16_t len);
static void              imu_gpio_init(void);
static void              imu_i2c_init(void);

extern void Error_Handler(void);

/* ══════════════════════════════════════════════════════════════════════
 *  IMU_Init
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Full init sequence:
 *   1. GPIO PB6/PB7 → AF4 Open-Drain
 *   2. I2C1 → Fast Mode 400 kHz
 *   3. Verify chip via WHO_AM_I (must return 0x68)
 *   4. Reset all signal paths
 *   5. Wake device, select best clock source
 *   6. Set DLPF, sample rate, gyro FS, accel FS
 *   7. Disable sleep on both power management registers
 */
HAL_StatusTypeDef IMU_Init(void)
{
    imu_gpio_init();
    imu_i2c_init();

    /* ── Verify chip is present ── */
    uint8_t who = 0;
    if (imu_read_regs(MPU_REG_WHO_AM_I, &who, 1U) != HAL_OK) return HAL_ERROR;
    if (who != MPU_WHO_AM_I_VAL)                              return HAL_ERROR;

    /* ── Full device reset — clears all registers to defaults ── */
    if (imu_write_reg(MPU_REG_PWR_MGMT_1, MPU_DEVICE_RESET) != HAL_OK) return HAL_ERROR;
    HAL_Delay(100U);  /* datasheet: wait ≥100 ms after reset */

    /* ── Reset signal paths (accel, gyro, temp ADCs) ── */
    if (imu_write_reg(MPU_REG_SIGNAL_PATH_RST, 0x07U) != HAL_OK) return HAL_ERROR;
    HAL_Delay(10U);

    /* ── Wake device + use PLL with X-axis gyro as clock source ──
     * PLL is more stable than internal 8 MHz oscillator.
     * This is the recommended clock source in the MPU6050 datasheet. */
    if (imu_write_reg(MPU_REG_PWR_MGMT_1, MPU_CLKSEL_PLL_XGYRO) != HAL_OK) return HAL_ERROR;

    /* ── DLPF configuration (CONFIG register) ──
     * DLPF_CFG = 2 → Gyro: 98 Hz BW / 3 ms delay
     *               Accel: 94 Hz BW / 3 ms delay
     * When DLPF is enabled, internal gyro output rate = 1 kHz.         */
    if (imu_write_reg(MPU_REG_CONFIG, IMU_DLPF_CONFIG) != HAL_OK) return HAL_ERROR;

    /* ── Sample rate divider ──
     * Sample Rate = Gyro Output Rate / (1 + SMPLRT_DIV)
     * With DLPF on, Gyro Output Rate = 1 kHz.
     * SMPLRT_DIV = 0 → Sample Rate = 1000 Hz                           */
    if (imu_write_reg(MPU_REG_SMPLRT_DIV, IMU_SAMPLE_RATE_DIV) != HAL_OK) return HAL_ERROR;

    /* ── Gyro full-scale range: ±2000 °/s ── */
    if (imu_write_reg(MPU_REG_GYRO_CONFIG, IMU_GYRO_CONFIG) != HAL_OK) return HAL_ERROR;

    /* ── Accel full-scale range: ±8 g ── */
    if (imu_write_reg(MPU_REG_ACCEL_CONFIG, IMU_ACCEL_CONFIG) != HAL_OK) return HAL_ERROR;

    /* ── Disable sleep, enable all axes ── */
    if (imu_write_reg(MPU_REG_PWR_MGMT_2, 0x00U) != HAL_OK) return HAL_ERROR;

    /* ── Disable interrupt pin (polled mode) ── */
    if (imu_write_reg(MPU_REG_INT_ENABLE, 0x00U) != HAL_OK) return HAL_ERROR;

    s_ready = true;
    return HAL_OK;
}

/* ══════════════════════════════════════════════════════════════════════
 *  IMU_Read
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Burst-read 14 bytes starting at ACCEL_XOUT_H (0x3B).
 * The MPU6050 auto-increments the register pointer after each byte,
 * so one I2C transaction reads accel + temp + gyro atomically.
 *
 * Byte order: big-endian (high byte first).
 * Reconstruct int16: val = (int16_t)((buf[2n] << 8) | buf[2n+1])
 */
HAL_StatusTypeDef IMU_Read(imu_data_t *dst)
{
    if (dst == NULL) return HAL_ERROR;

    uint8_t buf[BURST_LEN];

    if (imu_read_regs(MPU_REG_ACCEL_XOUT_H, buf, BURST_LEN) != HAL_OK)
        return HAL_ERROR;

    /* Reconstruct signed 16-bit values from big-endian byte pairs */
    int16_t raw_ax = (int16_t)((uint16_t)(buf[ 0] << 8U) | buf[ 1]);
    int16_t raw_ay = (int16_t)((uint16_t)(buf[ 2] << 8U) | buf[ 3]);
    int16_t raw_az = (int16_t)((uint16_t)(buf[ 4] << 8U) | buf[ 5]);
    int16_t raw_t  = (int16_t)((uint16_t)(buf[ 6] << 8U) | buf[ 7]);
    int16_t raw_gx = (int16_t)((uint16_t)(buf[ 8] << 8U) | buf[ 9]);
    int16_t raw_gy = (int16_t)((uint16_t)(buf[10] << 8U) | buf[11]);
    int16_t raw_gz = (int16_t)((uint16_t)(buf[12] << 8U) | buf[13]);

    /* Scale to physical units:
     * Accel: counts / sensitivity(LSB/g) → g → × g(m/s²)
     * Gyro:  counts / sensitivity(LSB/°/s) → °/s
     * Temp:  (counts / 340) + 36.53 → °C                              */
    dst->accel_x = ((float)raw_ax / MPU_ACCEL_SENS_8G)  * GRAVITY_MSS;
    dst->accel_y = ((float)raw_ay / MPU_ACCEL_SENS_8G)  * GRAVITY_MSS;
    dst->accel_z = ((float)raw_az / MPU_ACCEL_SENS_8G)  * GRAVITY_MSS;
    dst->gyro_x  =  (float)raw_gx / MPU_GYRO_SENS_2000;
    dst->gyro_y  =  (float)raw_gy / MPU_GYRO_SENS_2000;
    dst->gyro_z  =  (float)raw_gz / MPU_GYRO_SENS_2000;
    dst->temp_c  =  (float)raw_t  / MPU_TEMP_SENS + MPU_TEMP_OFFSET;

    return HAL_OK;
}

HAL_StatusTypeDef IMU_ReadRaw(imu_raw_t *dst)
{
    if (dst == NULL) return HAL_ERROR;

    uint8_t buf[BURST_LEN];
    if (imu_read_regs(MPU_REG_ACCEL_XOUT_H, buf, BURST_LEN) != HAL_OK)
        return HAL_ERROR;

    dst->ax   = (int16_t)((uint16_t)(buf[ 0] << 8U) | buf[ 1]);
    dst->ay   = (int16_t)((uint16_t)(buf[ 2] << 8U) | buf[ 3]);
    dst->az   = (int16_t)((uint16_t)(buf[ 4] << 8U) | buf[ 5]);
    dst->temp = (int16_t)((uint16_t)(buf[ 6] << 8U) | buf[ 7]);
    dst->gx   = (int16_t)((uint16_t)(buf[ 8] << 8U) | buf[ 9]);
    dst->gy   = (int16_t)((uint16_t)(buf[10] << 8U) | buf[11]);
    dst->gz   = (int16_t)((uint16_t)(buf[12] << 8U) | buf[13]);

    return HAL_OK;
}

/* ══════════════════════════════════════════════════════════════════════
 *  IMU_Calibrate
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Collect IMU_CALIB_SAMPLES (512) readings while the drone is perfectly
 * still and level.  Average them to compute per-axis bias offsets.
 *
 * Expected values when stationary & level:
 *   gyro_x, gyro_y, gyro_z  ≈ 0 °/s  (any offset is bias error)
 *   accel_x, accel_y        ≈ 0 m/s² (any offset is bias error)
 *   accel_z                 ≈ +9.81 m/s² (pointing up against gravity)
 *
 * We store the full measured mean as s_bias.  IMU_ApplyCalibration()
 * subtracts it, then adds back +9.81 on Z so that level-hover gives 0.
 * (i.e., calibrated accel_z = 0 m/s² means no vertical acceleration.)
 */
HAL_StatusTypeDef IMU_Calibrate(void)
{
    imu_data_t sample;
    double sum_ax = 0, sum_ay = 0, sum_az = 0;
    double sum_gx = 0, sum_gy = 0, sum_gz = 0;

    for (uint32_t i = 0; i < IMU_CALIB_SAMPLES; i++)
    {
        if (IMU_Read(&sample) != HAL_OK) return HAL_ERROR;

        sum_ax += (double)sample.accel_x;
        sum_ay += (double)sample.accel_y;
        sum_az += (double)sample.accel_z;
        sum_gx += (double)sample.gyro_x;
        sum_gy += (double)sample.gyro_y;
        sum_gz += (double)sample.gyro_z;

        HAL_Delay(1U);  /* ~1 kHz pace */
    }

    const float n = (float)IMU_CALIB_SAMPLES;

    /* Store mean of each axis as the bias */
    s_bias.accel_x = (float)(sum_ax / n);
    s_bias.accel_y = (float)(sum_ay / n);
    s_bias.accel_z = (float)(sum_az / n);   /* e.g. ≈ +9.81 + sensor offset */
    s_bias.gyro_x  = (float)(sum_gx / n);
    s_bias.gyro_y  = (float)(sum_gy / n);
    s_bias.gyro_z  = (float)(sum_gz / n);

    return HAL_OK;
}

/* ══════════════════════════════════════════════════════════════════════
 *  IMU_ApplyCalibration
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Remove stored biases from a fresh reading.
 *
 * For gyro: calibrated = measured - bias  (should be ~0 when still)
 * For accel X/Y: same subtraction.
 * For accel Z:   calibrated = measured - bias + GRAVITY_MSS
 *   → At rest pointing up:  measured ≈ bias  →  calibrated ≈ 0 m/s²
 *   → In free-fall:         measured ≈ 0     →  calibrated ≈ -9.81 m/s²
 *   This convention matches NED (North-East-Down) body-frame.
 */
void IMU_ApplyCalibration(imu_data_t *data)
{
    if (data == NULL) return;

    data->accel_x -= s_bias.accel_x;
    data->accel_y -= s_bias.accel_y;
    data->accel_z  = data->accel_z - s_bias.accel_z + GRAVITY_MSS;

    data->gyro_x  -= s_bias.gyro_x;
    data->gyro_y  -= s_bias.gyro_y;
    data->gyro_z  -= s_bias.gyro_z;
}

bool IMU_IsReady(void)
{
    return s_ready;
}

/* ══════════════════════════════════════════════════════════════════════
 *  Private: GPIO init
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Configure PB6 (SCL) and PB7 (SDA) as:
 *   - Alternate Function Open-Drain (mandatory for I2C)
 *   - No internal pull (board has external 4.7 kΩ pull-ups to 3.3V)
 *   - High speed (reduces rise-time issues at 400 kHz)
 *   - AF4 = I2C1 on STM32F405
 */
static void imu_gpio_init(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin       = I2C1_SCL_PIN | I2C1_SDA_PIN;  /* PB6 | PB7 */
    gpio.Mode      = GPIO_MODE_AF_OD;               /* open-drain */
    gpio.Pull      = GPIO_NOPULL;                   /* external pull-ups */
    gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
    gpio.Alternate = GPIO_AF4_I2C1;

    HAL_GPIO_Init(GPIOB, &gpio);
}

/* ══════════════════════════════════════════════════════════════════════
 *  Private: I2C1 peripheral init
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * I2C1 Fast Mode 400 kHz.
 * HAL calculates CCR and TRISE from ClockSpeed + APB1 frequency
 * (stored in HAL's internal RCC state after SystemClock_Config).
 */
static void imu_i2c_init(void)
{
    __HAL_RCC_I2C1_CLK_ENABLE();

    s_hi2c1.Instance             = I2C1;
    s_hi2c1.Init.ClockSpeed      = 400000U;           /* 400 kHz Fast Mode */
    s_hi2c1.Init.DutyCycle       = I2C_DUTYCYCLE_2;   /* 50% duty          */
    s_hi2c1.Init.OwnAddress1     = 0U;
    s_hi2c1.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
    s_hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    s_hi2c1.Init.OwnAddress2     = 0U;
    s_hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    s_hi2c1.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&s_hi2c1) != HAL_OK)
    {
        Error_Handler();
    }
}

/* ══════════════════════════════════════════════════════════════════════
 *  Private: register read/write helpers
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Write one byte to a MPU6050 register.
 * @param  reg  Register address.
 * @param  val  Value to write.
 */
static HAL_StatusTypeDef imu_write_reg(uint8_t reg, uint8_t val)
{
    return HAL_I2C_Mem_Write(
        &s_hi2c1,
        MPU6050_HAL_ADDR,       /* 8-bit addr = 0x68 << 1 = 0xD0 */
        reg,
        I2C_MEMADD_SIZE_8BIT,
        &val,
        1U,
        IMU_I2C_TIMEOUT_MS);
}

/**
 * @brief  Read one or more consecutive registers via auto-increment.
 * @param  reg  Starting register address.
 * @param  buf  Output buffer.
 * @param  len  Number of bytes to read.
 */
static HAL_StatusTypeDef imu_read_regs(uint8_t reg, uint8_t *buf, uint16_t len)
{
    return HAL_I2C_Mem_Read(
        &s_hi2c1,
        MPU6050_HAL_ADDR,
        reg,
        I2C_MEMADD_SIZE_8BIT,
        buf,
        len,
        IMU_I2C_TIMEOUT_MS);
}
