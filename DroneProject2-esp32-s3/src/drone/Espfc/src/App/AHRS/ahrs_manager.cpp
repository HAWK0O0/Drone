#include "AHRS/ahrs_manager.h"

#include <Adafruit_BNO055.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <math.h>

#include "Control/controller_math.h"
#include "INPUT/drone_config.h"

namespace {

enum class ImuBackend : uint8_t {
  kNone = 0,
  kMpu6050,
  kBno055,
};

Adafruit_MPU6050 g_mpu;
Adafruit_BNO055 g_bno_a(55, DroneConfig::kBnoAddressA, &Wire);
Adafruit_BNO055 g_bno_b(55, DroneConfig::kBnoAddressB, &Wire);
Adafruit_BNO055* g_bno = nullptr;

ImuBackend g_backend = ImuBackend::kNone;
ImuSnapshot g_snapshot{};

uint32_t g_last_read_us = 0;
uint32_t g_last_calibration_ms = 0;
float g_roll_offset_deg = 0.0f;
float g_pitch_offset_deg = 0.0f;
float g_raw_roll_deg = 0.0f;
float g_raw_pitch_deg = 0.0f;
float g_raw_yaw_deg = 0.0f;

float g_mpu_roll_deg = 0.0f;
float g_mpu_pitch_deg = 0.0f;
float g_mpu_yaw_deg = 0.0f;
float g_mpu_gyro_bias_x_dps = 0.0f;
float g_mpu_gyro_bias_y_dps = 0.0f;
float g_mpu_gyro_bias_z_dps = 0.0f;
bool g_mpu_filter_seeded = false;
bool g_mpu_calibrated = false;

constexpr uint8_t kMinCalSystem = 1;
constexpr uint8_t kMinCalGyro = 2;
constexpr uint8_t kMinCalAccel = 2;
constexpr uint32_t kBootCalibrationWaitMs = 7000;
constexpr uint32_t kBootCalibrationPollMs = 30;
constexpr uint8_t kLevelCaptureSamples = 24;
constexpr uint32_t kLevelCaptureDelayMs = 12;
constexpr float kRadToDeg = 57.2957795f;
constexpr float kDefaultDtSeconds = 0.01f;

bool ReadBnoEvents(sensors_event_t* euler, sensors_event_t* gyro) {
  if (g_bno == nullptr || euler == nullptr || gyro == nullptr) return false;

  g_bno->getEvent(euler, Adafruit_BNO055::VECTOR_EULER);
  g_bno->getEvent(gyro, Adafruit_BNO055::VECTOR_GYROSCOPE);

  return isfinite(euler->orientation.x) && isfinite(euler->orientation.y) &&
         isfinite(euler->orientation.z) && isfinite(gyro->gyro.x) &&
         isfinite(gyro->gyro.y) && isfinite(gyro->gyro.z);
}

bool ReadMpuEvents(sensors_event_t* accel, sensors_event_t* gyro, sensors_event_t* temp) {
  if (accel == nullptr || gyro == nullptr || temp == nullptr) return false;

  g_mpu.getEvent(accel, gyro, temp);
  return isfinite(accel->acceleration.x) && isfinite(accel->acceleration.y) &&
         isfinite(accel->acceleration.z) && isfinite(gyro->gyro.x) &&
         isfinite(gyro->gyro.y) && isfinite(gyro->gyro.z);
}

void ComputeMpuAccelAngles(const sensors_event_t& accel, float* roll_deg, float* pitch_deg) {
  const float accel_x = accel.acceleration.x;
  const float accel_y = accel.acceleration.y;
  const float accel_z = accel.acceleration.z;
  const float roll_den = fmaxf(0.001f, sqrtf(accel_x * accel_x + accel_z * accel_z));
  const float pitch_den = fmaxf(0.001f, sqrtf(accel_y * accel_y + accel_z * accel_z));

  if (roll_deg != nullptr) {
    *roll_deg = atan2f(accel_y, roll_den) * kRadToDeg * DroneConfig::kRollSign;
  }
  if (pitch_deg != nullptr) {
    *pitch_deg = atan2f(accel_x, pitch_den) * kRadToDeg * DroneConfig::kPitchSign;
  }
}

void ApplyBnoEvents(const sensors_event_t& euler, const sensors_event_t& gyro) {
  g_raw_yaw_deg = euler.orientation.x * DroneConfig::kYawSign;
  g_raw_roll_deg = euler.orientation.z * DroneConfig::kRollSign;
  g_raw_pitch_deg = euler.orientation.y * DroneConfig::kPitchSign;

  g_snapshot.yaw_deg = ControllerMath::WrapDegrees180(g_raw_yaw_deg);
  g_snapshot.roll_deg = g_raw_roll_deg - g_roll_offset_deg;
  g_snapshot.pitch_deg = g_raw_pitch_deg - g_pitch_offset_deg;
  g_snapshot.roll_rate_dps = gyro.gyro.y * kRadToDeg * DroneConfig::kRollSign;
  g_snapshot.pitch_rate_dps = gyro.gyro.x * kRadToDeg * DroneConfig::kPitchSign;
  g_snapshot.yaw_rate_dps = gyro.gyro.z * kRadToDeg * DroneConfig::kYawSign;
}

void ApplyMpuEvents(const sensors_event_t& accel, const sensors_event_t& gyro, float dt_seconds) {
  float accel_roll_deg = 0.0f;
  float accel_pitch_deg = 0.0f;
  ComputeMpuAccelAngles(accel, &accel_roll_deg, &accel_pitch_deg);

  const float roll_rate_dps =
      (gyro.gyro.y * kRadToDeg - g_mpu_gyro_bias_y_dps) * DroneConfig::kRollSign;
  const float pitch_rate_dps =
      (gyro.gyro.x * kRadToDeg - g_mpu_gyro_bias_x_dps) * DroneConfig::kPitchSign;
  const float yaw_rate_dps =
      (gyro.gyro.z * kRadToDeg - g_mpu_gyro_bias_z_dps) * DroneConfig::kYawSign;

  if (!g_mpu_filter_seeded) {
    g_mpu_roll_deg = accel_roll_deg;
    g_mpu_pitch_deg = accel_pitch_deg;
    g_mpu_yaw_deg = 0.0f;
    g_mpu_filter_seeded = true;
  } else {
    if (!(dt_seconds > 0.0f) || dt_seconds > 0.2f) {
      dt_seconds = kDefaultDtSeconds;
    }

    const float alpha = ControllerMath::Clamp(DroneConfig::kImuFilterAlpha, 0.0f, 1.0f);
    g_mpu_roll_deg += roll_rate_dps * dt_seconds;
    g_mpu_pitch_deg += pitch_rate_dps * dt_seconds;
    g_mpu_yaw_deg = ControllerMath::WrapDegrees180(g_mpu_yaw_deg + yaw_rate_dps * dt_seconds);

    g_mpu_roll_deg = alpha * g_mpu_roll_deg + (1.0f - alpha) * accel_roll_deg;
    g_mpu_pitch_deg = alpha * g_mpu_pitch_deg + (1.0f - alpha) * accel_pitch_deg;
  }

  g_raw_roll_deg = g_mpu_roll_deg;
  g_raw_pitch_deg = g_mpu_pitch_deg;
  g_raw_yaw_deg = g_mpu_yaw_deg;

  g_snapshot.roll_deg = g_raw_roll_deg - g_roll_offset_deg;
  g_snapshot.pitch_deg = g_raw_pitch_deg - g_pitch_offset_deg;
  g_snapshot.yaw_deg = ControllerMath::WrapDegrees180(g_raw_yaw_deg);
  g_snapshot.roll_rate_dps = roll_rate_dps;
  g_snapshot.pitch_rate_dps = pitch_rate_dps;
  g_snapshot.yaw_rate_dps = yaw_rate_dps;
}

bool InitBno() {
  g_bno = &g_bno_a;
  if (!g_bno->begin(OPERATION_MODE_IMUPLUS)) {
    g_bno = &g_bno_b;
    if (!g_bno->begin(OPERATION_MODE_IMUPLUS)) {
      g_bno = nullptr;
      return false;
    }
  }

  g_bno->setExtCrystalUse(DroneConfig::kBnoUseExternalCrystal);
  g_backend = ImuBackend::kBno055;
  return true;
}

bool CalibrateMpu() {
  float gyro_x_sum = 0.0f;
  float gyro_y_sum = 0.0f;
  float gyro_z_sum = 0.0f;
  float roll_sum = 0.0f;
  float pitch_sum = 0.0f;
  float max_rate_dps = 0.0f;

  for (uint16_t i = 0; i < DroneConfig::kMpu6050CalibrationSamples; ++i) {
    sensors_event_t accel{};
    sensors_event_t gyro{};
    sensors_event_t temp{};
    if (!ReadMpuEvents(&accel, &gyro, &temp)) {
      return false;
    }

    const float gyro_x_dps = gyro.gyro.x * kRadToDeg;
    const float gyro_y_dps = gyro.gyro.y * kRadToDeg;
    const float gyro_z_dps = gyro.gyro.z * kRadToDeg;
    float roll_deg = 0.0f;
    float pitch_deg = 0.0f;
    ComputeMpuAccelAngles(accel, &roll_deg, &pitch_deg);

    gyro_x_sum += gyro_x_dps;
    gyro_y_sum += gyro_y_dps;
    gyro_z_sum += gyro_z_dps;
    roll_sum += roll_deg;
    pitch_sum += pitch_deg;
    max_rate_dps = fmaxf(max_rate_dps,
                         fmaxf(fabsf(gyro_x_dps), fmaxf(fabsf(gyro_y_dps), fabsf(gyro_z_dps))));
    delay(DroneConfig::kMpu6050CalibrationDelayMs);
  }

  const float sample_count = static_cast<float>(DroneConfig::kMpu6050CalibrationSamples);
  g_mpu_gyro_bias_x_dps = gyro_x_sum / sample_count;
  g_mpu_gyro_bias_y_dps = gyro_y_sum / sample_count;
  g_mpu_gyro_bias_z_dps = gyro_z_sum / sample_count;
  g_mpu_roll_deg = roll_sum / sample_count;
  g_mpu_pitch_deg = pitch_sum / sample_count;
  g_mpu_yaw_deg = 0.0f;
  g_mpu_filter_seeded = true;

  return max_rate_dps <= DroneConfig::kMpu6050StillGyroThresholdDps;
}

bool InitMpu() {
  const bool found_primary = g_mpu.begin(DroneConfig::kMpu6050AddressA, &Wire, 0);
  const bool found_secondary = !found_primary &&
                               g_mpu.begin(DroneConfig::kMpu6050AddressB, &Wire, 0);
  if (!found_primary && !found_secondary) {
    return false;
  }

  g_mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  g_mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  g_mpu.setFilterBandwidth(MPU6050_BAND_44_HZ);
  delay(120);

  g_mpu_filter_seeded = false;
  g_mpu_calibrated = CalibrateMpu();
  g_backend = ImuBackend::kMpu6050;
  return true;
}

void UpdateCalibrationStatus() {
  if (!g_snapshot.ready) {
    g_snapshot.calibrated = false;
    g_snapshot.cal_sys = 0;
    g_snapshot.cal_gyro = 0;
    g_snapshot.cal_accel = 0;
    g_snapshot.cal_mag = 0;
    return;
  }

  if (g_backend == ImuBackend::kBno055 && g_bno != nullptr) {
    g_bno->getCalibration(&g_snapshot.cal_sys, &g_snapshot.cal_gyro, &g_snapshot.cal_accel,
                          &g_snapshot.cal_mag);

    const bool imuplus_ready = g_snapshot.cal_gyro >= kMinCalGyro &&
                               g_snapshot.cal_accel >= kMinCalAccel;
    if (imuplus_ready) {
      g_snapshot.cal_sys = 3;
      g_snapshot.cal_mag = 3;
    }

    g_snapshot.calibrated = imuplus_ready ||
                            (g_snapshot.cal_sys >= kMinCalSystem &&
                             g_snapshot.cal_gyro >= kMinCalGyro &&
                             g_snapshot.cal_accel >= kMinCalAccel);
    return;
  }

  if (g_backend == ImuBackend::kMpu6050) {
    g_snapshot.cal_gyro = g_mpu_filter_seeded ? (g_mpu_calibrated ? 3 : 2) : 0;
    g_snapshot.cal_accel = g_mpu_filter_seeded ? 3 : 0;
    g_snapshot.cal_mag = 0;
    g_snapshot.cal_sys = g_mpu_calibrated ? 3 : (g_mpu_filter_seeded ? 2 : 0);
    g_snapshot.calibrated = g_mpu_calibrated;
    return;
  }

  g_snapshot.calibrated = false;
  g_snapshot.cal_sys = 0;
  g_snapshot.cal_gyro = 0;
  g_snapshot.cal_accel = 0;
  g_snapshot.cal_mag = 0;
}

bool CaptureLevelFromBno(float* roll_offset_deg, float* pitch_offset_deg) {
  float roll_sum = 0.0f;
  float pitch_sum = 0.0f;
  uint8_t sample_count = 0;
  for (uint8_t i = 0; i < kLevelCaptureSamples; ++i) {
    sensors_event_t euler{};
    sensors_event_t gyro{};
    if (ReadBnoEvents(&euler, &gyro)) {
      roll_sum += euler.orientation.z * DroneConfig::kRollSign;
      pitch_sum += euler.orientation.y * DroneConfig::kPitchSign;
      ++sample_count;
    }
    delay(kLevelCaptureDelayMs);
  }

  if (sample_count == 0) return false;
  *roll_offset_deg = roll_sum / static_cast<float>(sample_count);
  *pitch_offset_deg = pitch_sum / static_cast<float>(sample_count);
  return true;
}

bool CaptureLevelFromMpu(float* roll_offset_deg, float* pitch_offset_deg) {
  float roll_sum = 0.0f;
  float pitch_sum = 0.0f;
  uint8_t sample_count = 0;
  for (uint8_t i = 0; i < kLevelCaptureSamples; ++i) {
    sensors_event_t accel{};
    sensors_event_t gyro{};
    sensors_event_t temp{};
    if (ReadMpuEvents(&accel, &gyro, &temp)) {
      float roll_deg = 0.0f;
      float pitch_deg = 0.0f;
      ComputeMpuAccelAngles(accel, &roll_deg, &pitch_deg);
      roll_sum += roll_deg;
      pitch_sum += pitch_deg;
      ++sample_count;
    }
    delay(kLevelCaptureDelayMs);
  }

  if (sample_count == 0) return false;
  *roll_offset_deg = roll_sum / static_cast<float>(sample_count);
  *pitch_offset_deg = pitch_sum / static_cast<float>(sample_count);
  return true;
}

void CaptureStartupLevel() {
  if (!g_snapshot.ready) return;

  g_roll_offset_deg = 0.0f;
  g_pitch_offset_deg = 0.0f;

  float roll_offset_deg = 0.0f;
  float pitch_offset_deg = 0.0f;
  const bool captured =
      (g_backend == ImuBackend::kBno055)
          ? CaptureLevelFromBno(&roll_offset_deg, &pitch_offset_deg)
          : (g_backend == ImuBackend::kMpu6050)
                ? CaptureLevelFromMpu(&roll_offset_deg, &pitch_offset_deg)
                : false;

  if (captured) {
    g_roll_offset_deg = roll_offset_deg;
    g_pitch_offset_deg = pitch_offset_deg;
  }
}

}  // namespace

void ImuManager::begin(float roll_offset_deg, float pitch_offset_deg) {
  (void)roll_offset_deg;
  (void)pitch_offset_deg;

  g_snapshot = {};
  g_backend = ImuBackend::kNone;
  g_bno = nullptr;
  g_roll_offset_deg = 0.0f;
  g_pitch_offset_deg = 0.0f;
  g_raw_roll_deg = 0.0f;
  g_raw_pitch_deg = 0.0f;
  g_raw_yaw_deg = 0.0f;
  g_mpu_roll_deg = 0.0f;
  g_mpu_pitch_deg = 0.0f;
  g_mpu_yaw_deg = 0.0f;
  g_mpu_gyro_bias_x_dps = 0.0f;
  g_mpu_gyro_bias_y_dps = 0.0f;
  g_mpu_gyro_bias_z_dps = 0.0f;
  g_mpu_filter_seeded = false;
  g_mpu_calibrated = false;

  Wire.begin(DroneConfig::kI2cSdaPin, DroneConfig::kI2cSclPin, 400000U);

  bool ready = false;
  if (DroneConfig::kPreferMpu6050) {
    ready = InitMpu() || InitBno();
  } else {
    ready = InitBno() || InitMpu();
  }

  g_snapshot.ready = ready;
  g_last_read_us = micros();
  g_last_calibration_ms = millis();

  if (!g_snapshot.ready) {
    UpdateCalibrationStatus();
    return;
  }

  if (g_backend == ImuBackend::kBno055) {
    const uint32_t wait_start_ms = millis();
    while ((millis() - wait_start_ms) < kBootCalibrationWaitMs) {
      UpdateCalibrationStatus();
      if (g_snapshot.calibrated) break;
      delay(kBootCalibrationPollMs);
    }

    sensors_event_t euler{};
    sensors_event_t gyro{};
    if (ReadBnoEvents(&euler, &gyro)) {
      ApplyBnoEvents(euler, gyro);
    }
  } else if (g_backend == ImuBackend::kMpu6050) {
    sensors_event_t accel{};
    sensors_event_t gyro{};
    sensors_event_t temp{};
    if (ReadMpuEvents(&accel, &gyro, &temp)) {
      ApplyMpuEvents(accel, gyro,
                     static_cast<float>(DroneConfig::kImuReadPeriodUs) / 1000000.0f);
    }
  }

  UpdateCalibrationStatus();
  CaptureStartupLevel();
  g_last_read_us = micros();
}

void ImuManager::update() {
  if (!g_snapshot.ready || g_backend == ImuBackend::kNone) return;

  const uint32_t now_us = micros();
  if ((now_us - g_last_read_us) < DroneConfig::kImuReadPeriodUs) return;

  const float dt_seconds = static_cast<float>(now_us - g_last_read_us) / 1000000.0f;
  g_last_read_us = now_us;

  if (g_backend == ImuBackend::kBno055) {
    sensors_event_t euler{};
    sensors_event_t gyro{};
    if (ReadBnoEvents(&euler, &gyro)) {
      ApplyBnoEvents(euler, gyro);
    }
  } else if (g_backend == ImuBackend::kMpu6050) {
    sensors_event_t accel{};
    sensors_event_t gyro{};
    sensors_event_t temp{};
    if (ReadMpuEvents(&accel, &gyro, &temp)) {
      ApplyMpuEvents(accel, gyro, dt_seconds);
    }
  }

  if ((millis() - g_last_calibration_ms) >= 250) {
    g_last_calibration_ms = millis();
    UpdateCalibrationStatus();
  }
}

ImuSnapshot ImuManager::snapshot() {
  return g_snapshot;
}

bool ImuManager::isReady() {
  return g_snapshot.ready;
}

bool ImuManager::captureLevelOffsets(float* roll_offset_deg, float* pitch_offset_deg) {
  if (!g_snapshot.ready || roll_offset_deg == nullptr || pitch_offset_deg == nullptr) {
    return false;
  }

  UpdateCalibrationStatus();
  if (!g_snapshot.calibrated) {
    return false;
  }

  if (g_backend == ImuBackend::kBno055) {
    return CaptureLevelFromBno(roll_offset_deg, pitch_offset_deg);
  }
  if (g_backend == ImuBackend::kMpu6050) {
    return CaptureLevelFromMpu(roll_offset_deg, pitch_offset_deg);
  }
  return false;
}

void ImuManager::setLevelOffsets(float roll_offset_deg, float pitch_offset_deg) {
  g_roll_offset_deg = roll_offset_deg;
  g_pitch_offset_deg = pitch_offset_deg;
}

void ImuManager::getLevelOffsets(float* roll_offset_deg, float* pitch_offset_deg) {
  if (roll_offset_deg != nullptr) *roll_offset_deg = g_roll_offset_deg;
  if (pitch_offset_deg != nullptr) *pitch_offset_deg = g_pitch_offset_deg;
}
