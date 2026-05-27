/**
 * @file    imu.h
 * @brief   IMU driver — MPU6050 via I2C1 (PB6=SCL, PB7=SDA) @ 400 kHz
 *
 * ── Chip overview ────────────────────────────────────────────────────
 *   MPU6050: 3-axis gyroscope + 3-axis accelerometer + temperature
 *   I2C address: 0x68 (AD0=GND) or 0x69 (AD0=VCC)
 *   WHO_AM_I register (0x75) returns 0x68 always
 *
 * ── Configuration (compile-time) ─────────────────────────────────────
 *   Gyro  full-scale : ±2000 °/s   → sensitivity 16.4  LSB/(°/s)
 *   Accel full-scale : ±8 g        → sensitivity  4096 LSB/g
 *   DLPF bandwidth   : ~98 Hz gyro / ~94 Hz accel (DLPF_CFG = 2)
 *   Sample rate      : 1000 Hz     (SMPLRT_DIV = 0)
 *
 * ── I2C bus ──────────────────────────────────────────────────────────
 *   Peripheral : I2C1
 *   SCL        : PB6  AF4
 *   SDA        : PB7  AF4
 *   Speed      : Fast Mode 400 kHz
 *   Pull-ups   : external 4.7 kΩ on board (GPIO set to NOPULL)
 *
 * ── Coordinate system ────────────────────────────────────────────────
 *   X: roll axis  (nose→left is positive gyro_x, right wing down = +accel_x)
 *   Y: pitch axis (nose down is positive gyro_y)
 *   Z: yaw axis   (counter-clockwise from above is positive gyro_z)
 *
 * ── Usage ─────────────────────────────────────────────────────────────
 *   IMU_Init();            // once, after SystemClock_Config
 *   IMU_Calibrate();       // once, drone flat & still (~1 s)
 *   while (1) {
 *       IMU_Read(&data);
 *       IMU_ApplyCalibration(&data);
 *       // use data.gyro_x / accel_z …
 *   }
 */

#ifndef FC_IMU_H
#define FC_IMU_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* ── I2C address (7-bit) ──────────────────────────────────────────── */
#define MPU6050_ADDR_LOW    0x68U   /**< AD0 pin = GND (default)       */
#define MPU6050_ADDR_HIGH   0x69U   /**< AD0 pin = VCC                 */

/* HAL expects the 8-bit form (7-bit << 1) */
#define MPU6050_HAL_ADDR    (MPU6050_ADDR_LOW << 1U)

/* ── MPU6050 register map ────────────────────────────────────────── */
#define MPU_REG_SMPLRT_DIV      0x19U  /**< Sample Rate Divider        */
#define MPU_REG_CONFIG          0x1AU  /**< DLPF config                */
#define MPU_REG_GYRO_CONFIG     0x1BU  /**< Gyro full-scale select     */
#define MPU_REG_ACCEL_CONFIG    0x1CU  /**< Accel full-scale select    */
#define MPU_REG_INT_ENABLE      0x38U  /**< Interrupt enable           */
#define MPU_REG_ACCEL_XOUT_H    0x3BU  /**< First data register        */
#define MPU_REG_TEMP_OUT_H      0x41U  /**< Temperature high byte      */
#define MPU_REG_GYRO_XOUT_H     0x43U  /**< Gyro data start            */
#define MPU_REG_SIGNAL_PATH_RST 0x68U  /**< Signal path reset          */
#define MPU_REG_USER_CTRL       0x6AU  /**< User control               */
#define MPU_REG_PWR_MGMT_1      0x6BU  /**< Power management 1        */
#define MPU_REG_PWR_MGMT_2      0x6CU  /**< Power management 2        */
#define MPU_REG_WHO_AM_I        0x75U  /**< Returns 0x68 always        */

/* ── Register bit-field values ───────────────────────────────────── */
#define MPU_WHO_AM_I_VAL        0x68U  /**< Expected WHO_AM_I response */

/* PWR_MGMT_1 */
#define MPU_CLKSEL_PLL_XGYRO    0x01U  /**< PLL with X Gyro ref (best) */
#define MPU_DEVICE_RESET        0x80U  /**< Full device reset          */
#define MPU_SLEEP_DISABLE       0x00U  /**< Wake device                */

/* GYRO_CONFIG — FS_SEL bits [4:3] */
#define MPU_GYRO_FS_250         0x00U  /**< ±250  °/s — 131.0 LSB/°/s */
#define MPU_GYRO_FS_500         0x08U  /**< ±500  °/s —  65.5 LSB/°/s */
#define MPU_GYRO_FS_1000        0x10U  /**< ±1000 °/s —  32.8 LSB/°/s */
#define MPU_GYRO_FS_2000        0x18U  /**< ±2000 °/s —  16.4 LSB/°/s */

/* ACCEL_CONFIG — AFS_SEL bits [4:3] */
#define MPU_ACCEL_FS_2G         0x00U  /**< ±2  g — 16384 LSB/g       */
#define MPU_ACCEL_FS_4G         0x08U  /**< ±4  g —  8192 LSB/g       */
#define MPU_ACCEL_FS_8G         0x10U  /**< ±8  g —  4096 LSB/g       */
#define MPU_ACCEL_FS_16G        0x18U  /**< ±16 g —  2048 LSB/g       */

/* CONFIG — DLPF_CFG bits [2:0]
 * Higher values = lower bandwidth = more filtering = more delay.
 * DLPF_CFG=2: Gyro 98 Hz / 3 ms delay, Accel 94 Hz / 3 ms delay. */
#define MPU_DLPF_260HZ          0x00U
#define MPU_DLPF_184HZ          0x01U
#define MPU_DLPF_92HZ           0x02U  /**< Flight controller default  */
#define MPU_DLPF_41HZ           0x03U
#define MPU_DLPF_20HZ           0x04U
#define MPU_DLPF_10HZ           0x05U
#define MPU_DLPF_5HZ            0x06U

/* ── Sensitivity constants ───────────────────────────────────────── */
#define MPU_GYRO_SENS_2000      16.4f   /**< LSB / (°/s) at ±2000 °/s */
#define MPU_ACCEL_SENS_8G       4096.0f /**< LSB / g      at ±8 g     */
#define MPU_TEMP_SENS           340.0f  /**< LSB / °C                 */
#define MPU_TEMP_OFFSET         36.53f  /**< °C offset from datasheet  */

/* ── Compile-time configuration ──────────────────────────────────── */
#define IMU_GYRO_CONFIG         MPU_GYRO_FS_2000   /**< Active FS       */
#define IMU_ACCEL_CONFIG        MPU_ACCEL_FS_8G    /**< Active FS       */
#define IMU_DLPF_CONFIG         MPU_DLPF_92HZ      /**< Active DLPF     */
#define IMU_SAMPLE_RATE_DIV     0U                 /**< 1 kHz output    */
#define IMU_CALIB_SAMPLES       512U               /**< Calibration avg */
#define IMU_I2C_TIMEOUT_MS      10U                /**< Per-transfer    */

/* ── Data structures ─────────────────────────────────────────────── */

/**
 * @brief  Fully-scaled, calibrated IMU measurement.
 *         Produced by IMU_Read() → IMU_ApplyCalibration().
 */
typedef struct {
    float accel_x;   /**< Acceleration X  [m/s²]  */
    float accel_y;   /**< Acceleration Y  [m/s²]  */
    float accel_z;   /**< Acceleration Z  [m/s²]  */
    float gyro_x;    /**< Angular rate X  [°/s]   */
    float gyro_y;    /**< Angular rate Y  [°/s]   */
    float gyro_z;    /**< Angular rate Z  [°/s]   */
    float temp_c;    /**< Die temperature [°C]    */
} imu_data_t;

/**
 * @brief  Raw 16-bit values straight from the sensor registers.
 *         Useful for logging or custom scaling.
 */
typedef struct {
    int16_t ax, ay, az;  /**< Raw accel counts  */
    int16_t gx, gy, gz;  /**< Raw gyro counts   */
    int16_t temp;        /**< Raw temperature   */
} imu_raw_t;

/* ── Public API ──────────────────────────────────────────────────── */

/**
 * @brief  Initialise I2C1 GPIO + peripheral, reset and configure MPU6050.
 * @return HAL_OK on success, HAL_ERROR if chip not detected.
 */
HAL_StatusTypeDef IMU_Init(void);

/**
 * @brief  Read all sensor data in a single 14-byte burst.
 *         Converts raw counts to physical units.
 * @param  dst  Non-null pointer; filled with fresh data on HAL_OK.
 * @return HAL_OK on success, HAL_ERROR on I2C fault.
 */
HAL_StatusTypeDef IMU_Read(imu_data_t *dst);

/**
 * @brief  Read raw register values without scaling.
 * @param  dst  Non-null pointer.
 * @return HAL_OK on success.
 */
HAL_StatusTypeDef IMU_ReadRaw(imu_raw_t *dst);

/**
 * @brief  Collect IMU_CALIB_SAMPLES while drone is stationary.
 *         Computes and stores bias offsets.  Blocks ~0.5 s.
 * @return HAL_OK on success.
 */
HAL_StatusTypeDef IMU_Calibrate(void);

/**
 * @brief  Subtract stored bias from a measurement in-place.
 *         Call immediately after IMU_Read() every loop iteration.
 * @param  data  Pointer to the imu_data_t to correct.
 */
void IMU_ApplyCalibration(imu_data_t *data);

/**
 * @brief  Returns true if IMU_Init() succeeded and chip was detected.
 */
bool IMU_IsReady(void);

#endif /* FC_IMU_H */
