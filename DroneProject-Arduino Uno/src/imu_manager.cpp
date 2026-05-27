#include <Arduino.h>
#include <Wire.h>

#include <Adafruit_BNO055.h>
#include <Adafruit_Sensor.h>
#include "controller_config.h"
#include "imu_manager.h"

namespace {

// ============================================================================
// BNO055 九轴传感器 (陀螺仪 + 加速度计 + 磁力计)
// ============================================================================
// 尝试两种常见的 I2C 地址：
// - 某些分线板为 0x28
// - 其他分线板为 0x29
// 用户可以配置预期地址，程序将先后探测这两个地址。
Adafruit_BNO055 bnoA(55, Config::BNO_ADDR_A, &Wire);
Adafruit_BNO055 bnoB(55, Config::BNO_ADDR_B, &Wire);
Adafruit_BNO055 *bno = nullptr;  // 当前激活的传感器指针

// 连接与校准状态
bool bnoOk = false;               // 传感器是否就绪？
bool bnoCalOk = false;            // 校准是否达标？

// ============================================================================
// 用于自动调平模式的最新欧拉角 (Euler angles)
// ============================================================================
float imuRoll = 0.0f;             // 滚转角 (单位：度)
float imuPitch = 0.0f;            // 俯仰角 (单位：度)
float imuYaw = 0.0f;              // 偏航角 (单位：度)
bool imuFilterInitialized = false; // 滤波器是否已初始化？

// ============================================================================
// 定期更新定时器
// ============================================================================
uint32_t lastCalMs = 0;           // 上次检查校准状态的时间
uint32_t lastEulerMs = 0;         // 上次读取欧拉角的时间

// ============================================================================
// 测试指定地址上的传感器并启用外部晶振
// ============================================================================
// 如果地址均无响应，则返回 false，代码将避免使用该设备。
bool initBno() {
  bno = &bnoA;  // 首先尝试地址 A
  if (!bno->begin(Config::BNO_MODE)) {
    bno = &bnoB;  // 尝试地址 B
    if (!bno->begin(Config::BNO_MODE)) {
      bno = nullptr;
      return false;  // 两个地址均通信失败
    }
  }

  // 启用外部晶振以获得更高精度
  bno->setExtCrystalUse(true);
  return true;
}

// ============================================================================
// 读取校准状态并与 Config 阈值进行比较
// ============================================================================
void updateCalibration() {
  if (!bnoOk || !bno) {
    bnoCalOk = false;
    return;
  }

  uint8_t sys = 0;
  uint8_t gyro = 0;
  uint8_t accel = 0;
  uint8_t mag = 0;
  bno->getCalibration(&sys, &gyro, &accel, &mag);

  // 检查各模块校准值是否达到最小配置要求
  bnoCalOk = (sys >= Config::CAL_SYS_MIN) && (gyro >= Config::CAL_GYRO_MIN) &&
             (accel >= Config::CAL_ACCEL_MIN) && (mag >= Config::CAL_MAG_MIN);
}

}  // 命名空间 (namespace)

void ImuManager::begin() {
  // 在调用任何 BNO055 函数之前启动 I2C 通信
  Wire.begin();
  bnoOk = initBno();
  
#if BNO_DEBUG
  if (bnoOk) {
    Serial.println(F("BNO055 初始化成功！"));
  } else {
    Serial.println(F("BNO055 初始化失败 - 请检查 I2C 接线 (A4=SDA, A5=SCL)"));
  }
#endif
}

// 更新逻辑：
// - 每 200 毫秒更新一次校准状态
// - 每约 500 毫秒读取一次欧拉角用于诊断（无论自动调平状态如何）
void ImuManager::update(bool autoLevelEnabled) {
  if (millis() - lastCalMs >= 200) {
    lastCalMs = millis();
    updateCalibration();
  }

  // 当开启 BNO_DEBUG 时，始终读取并记录欧拉角用于诊断
#if BNO_DEBUG
  static uint32_t lastDiagMs = 0;
  if ((millis() - lastDiagMs) >= 500) {
    lastDiagMs = millis();
    if (bnoOk && bno) {
      sensors_event_t euler;
      bno->getEvent(&euler, Adafruit_BNO055::VECTOR_EULER);
      Serial.print(F("BNO 偏航: "));
      Serial.print(euler.orientation.x, 1);
      Serial.print(F(" 滚转: "));
      Serial.print(euler.orientation.y, 1);
      Serial.print(F(" 俯仰: "));
      Serial.print(euler.orientation.z, 1);
      Serial.print(F(" | 校准: "));
      uint8_t sys, gyro, accel, mag;
      bno->getCalibration(&sys, &gyro, &accel, &mag);
      Serial.print(F("系统="));
      Serial.print(sys, DEC);
      Serial.print(F(" 陀螺仪="));
      Serial.print(gyro, DEC);
      Serial.print(F(" 加速度="));
      Serial.print(accel, DEC);
      Serial.print(F(" 磁力="));
      Serial.println(mag, DEC);
    } else {
      Serial.println(F("BNO055 未就绪或未初始化"));
    }
  }
#endif

  // 仅在启用自动调平且传感器就绪时更新滤波器
  if (!(autoLevelEnabled && bnoOk && bno)) return;
  if ((millis() - lastEulerMs) < 20) return;

  lastEulerMs = millis();

  sensors_event_t euler;
  bno->getEvent(&euler, Adafruit_BNO055::VECTOR_EULER);

  // 应用轴向正负号配置
  float rawYaw = euler.orientation.x * Config::IMU_YAW_SIGN;
  float rawRoll = euler.orientation.y * Config::IMU_ROLL_SIGN;
  float rawPitch = euler.orientation.z * Config::IMU_PITCH_SIGN;

  // 限制低通滤波器系数 alpha 在 [0, 1] 之间
  float alpha = Config::IMU_EULER_LPF_ALPHA;
  if (alpha < 0.0f) alpha = 0.0f;
  if (alpha > 1.0f) alpha = 1.0f;

  if (!imuFilterInitialized) {
    imuYaw = rawYaw;
    imuRoll = rawRoll;
    imuPitch = rawPitch;
    imuFilterInitialized = true;
  } else {
    // 应用低通滤波 (LPF)
    imuYaw += alpha * (rawYaw - imuYaw);
    imuRoll += alpha * (rawRoll - imuRoll);
    imuPitch += alpha * (rawPitch - imuPitch);
  }
}

bool ImuManager::isReady() {
  return bnoOk;
}

bool ImuManager::calibrationOk() {
  return bnoCalOk;
}

float ImuManager::roll() {
  return imuRoll;
}

float ImuManager::pitch() {
  return imuPitch;
}

float ImuManager::yaw() {
  return imuYaw;
}