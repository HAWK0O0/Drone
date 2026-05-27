#include <Arduino.h>
#include "controller_config.h"
#include "controller_math.h"
#include "flight_controller.h"
#include "flight_mixer.h"
#include "gps_manager.h"
#include "imu_manager.h"
#include "motor_output.h"
#include "rc_input.h"

namespace {

// ============================================================================
// 模式标志（由遥控开关控制）
// ============================================================================
bool armed = false;                  // 系统是否解锁（已启动）？
bool autoLevelEnabled = false;       // 自动平衡是否启用？
bool balanceAssistEnabled = false;   // 平衡辅助是否启用？
bool testModeEnabled = false;        // 测试模式是否启用？

// ============================================================================
// 传感器状态（GPS 与 IMU）与故障保护
// ============================================================================
// 在 loop() 中管理；在此声明以便其他逻辑查询
bool sensorsEnabled = false;         // 传感器（IMU）是否启用？
bool sensorsFailLatch = false;       // 传感器失效保护（锁存）
bool gpsEnabled = false;             // GPS 是否启用？

}  // namespace

void FlightController::setup() {
  // ============================================================================
  // 系统初始化 - 启动顺序
  // ============================================================================
#if STATUS_DEBUG
  Serial.begin(115200);  // Serial 波特率用于调试输出
#endif

  // 初始化模块，顺序说明：
  // 1) GPS 串口 - 先初始化 GPS
  // 2) RC 输入 + 中断 - 配置遥控输入与中断
  // 3) ESC 输出（安全停止）- 初始化电调输出，默认安全停止
  // 4) IMU（I2C + BNO055）- 初始化姿态传感器
  GpsManager::begin();
  RcInput::begin();
  MotorOutput::begin();
  ImuManager::begin();
}

void FlightController::loop() {
  // ============================================================================
  // 更新传感器数据 - 仅在启用时执行
  // ============================================================================
  // 通过 CH5 开关选择传感器模式（禁用 / 仅 IMU / IMU + GPS）
  if (sensorsEnabled) {
    ImuManager::update(autoLevelEnabled);  // 更新姿态数据
  }
  if (gpsEnabled) {
    GpsManager::update();  // 更新 GPS 数据
  }

  // ============================================================================
  // 严重保护：检查遥控信号
  // ============================================================================
  // 如果遥控器的通道没有刷新，则立即断开解锁并停止所有电机
  if (!RcInput::frameFresh()) {
    armed = false;
    MotorOutput::writeStopAll();  // 停止所有电机
    return;
  }

  // ============================================================================
  // 读取并约束所有有效的遥控通道
  // ============================================================================
  // 遥杆映射：CH1=横滚(Roll), CH2=俯仰(Pitch), CH3=油门(Throttle), CH4=偏航(Yaw)
  
  // 横滚 (Roll) - 左/右
  uint16_t roll = ControllerMath::constrainU16(RcInput::read(1, Config::RC_MID), Config::RC_MIN, Config::RC_MAX);
  
  // 俯仰 (Pitch) - 前/后
  uint16_t pitch = ControllerMath::constrainU16(RcInput::read(2, Config::RC_MID), Config::RC_MIN, Config::RC_MAX);
  
  // 油门 (Throttle) - 电机转速
  uint16_t throttleIn = ControllerMath::constrainU16(RcInput::read(3, Config::RC_MIN), Config::RC_MIN, Config::RC_MAX);
  
  // 偏航 (Yaw) - 旋转
  uint16_t yaw = ControllerMath::constrainU16(RcInput::read(4, Config::RC_MID), Config::RC_MIN, Config::RC_MAX);
  
  // CH5: 传感器系统模式开关（三档）
  uint16_t autoLevelSwitch = ControllerMath::constrainU16(RcInput::read(5, Config::RC_MID), Config::RC_MIN, Config::RC_MAX);
  
  // CH6: 解锁/锁定 开关
  uint16_t armSwitch = ControllerMath::constrainU16(RcInput::read(6, Config::RC_MIN), Config::RC_MIN, Config::RC_MAX);
  
  // CH7: 备用通道（当前未使用）
  uint16_t ch7 = ControllerMath::constrainU16(RcInput::read(7, Config::RC_MID), Config::RC_MIN, Config::RC_MAX);
  (void)ch7;  // 抑制未使用变量警告
  
  // CH8: 测试模式开关
  uint16_t ch8 = ControllerMath::constrainU16(RcInput::read(8, Config::RC_MID), Config::RC_MIN, Config::RC_MAX);

  // ============================================================================
  // CH5：传感器三档开关说明
  // ============================================================================
  // 低位 (≈1000) -> 普通飞行，禁用传感器
  // 中位 (≈1500) -> 仅启用 IMU/罗盘
  // 高位 (≈2000) -> 启用 IMU + GPS
  if (RcInput::channelFresh(5)) {
    if (autoLevelSwitch <= Config::SW_OFF) {
      // 模式1：禁用传感器
      sensorsEnabled = false;
      gpsEnabled = false;
      sensorsFailLatch = false;
      GpsManager::setEnabled(false);
      balanceAssistEnabled = false;
      autoLevelEnabled = false;
      
    } else if (autoLevelSwitch >= Config::SW_ON) {
      // 模式3：启用 IMU + GPS
      bool imuReady = ImuManager::isReady();             // IMU 是否就绪？
      bool gpsOk = GpsManager::isHealthy();             // GPS 是否健康可用？
      
      if (imuReady && gpsOk) {
        sensorsEnabled = true;
        gpsEnabled = true;
        GpsManager::setEnabled(true);
        sensorsFailLatch = false;
      } else {
        sensorsEnabled = false;
        gpsEnabled = false;
        sensorsFailLatch = true;  // 启用故障保护
        GpsManager::setEnabled(false);
      }
      balanceAssistEnabled = false;
      autoLevelEnabled = sensorsEnabled;  // 仅当 IMU 可用时允许自动平衡
      
    } else {
      // 模式2：仅 IMU（不启用 GPS）
      if (ImuManager::isReady()) {
        sensorsEnabled = true;
        gpsEnabled = false;
        GpsManager::setEnabled(false);
        sensorsFailLatch = false;
      } else {
        sensorsEnabled = false;
        gpsEnabled = false;
        sensorsFailLatch = true;
        GpsManager::setEnabled(false);
      }
      balanceAssistEnabled = false;
      autoLevelEnabled = false;
    }
  }

  // ============================================================================
  // 传感器故障保护 - 若传感器不可用则自动停用相关功能
  // ============================================================================
  // 如果 IMU 不可用：
  if (sensorsEnabled) {
    if (!ImuManager::isReady()) {
      sensorsEnabled = false;
      sensorsFailLatch = true;
      GpsManager::setEnabled(false);
    }
  }
  
  // 如果 GPS 不可用：
  if (gpsEnabled) {
    if (!GpsManager::isHealthy()) {
      gpsEnabled = false;
      sensorsFailLatch = true;
      GpsManager::setEnabled(false);
    }
  }

  // ============================================================================
  // CH8：测试模式开关（用于单电机测试）
  // ============================================================================
  if (RcInput::channelFresh(8)) {
    testModeEnabled = RcInput::switchFromValue(ch8, testModeEnabled);
  }

  // ============================================================================
  // CH6：解锁/锁定 策略
  // ============================================================================
  // 高位 => 解锁（armed），低位 => 锁定（disarmed）
  if (armSwitch >= Config::ARM_SW_ON) {
    armed = true;
  } else if (armSwitch <= Config::ARM_SW_OFF) {
    armed = false;
  }

  uint16_t throttleEsc = FlightMixer::mapThrottleToEsc(throttleIn);

  // 未解锁时绝不发送非零输出。
  if (!armed) {
    MotorOutput::writeStopAll();
    return;
  }

  // 诊断：向所有 ESC 输出相同信号。
  // 用于排查单个引脚或 ESC 启动延迟问题。
  if (Config::ESC_DIAG_EQUAL_OUTPUTS) {
    uint16_t diagUs = ControllerMath::constrainU16(
        Config::ESC_DIAG_OUTPUT_US, Config::ESC_STOP, Config::ESC_MAX);
    MotorOutput::write(diagUs, diagUs, diagUs, diagUs);
    return;
  }

  // 可选直接模式：
  // 将油门直接发送到所有电机（仅用于台架检查）。
  if (Config::DIRECT_ESC_PASSTHROUGH) {
    MotorOutput::write(FlightMixer::passthroughOutputs(throttleEsc));
    return;
  }

  // 测试模式：
  // 根据横滚杆位置选择单个电机并驱动，仅用于测试。
  if (testModeEnabled) {
    MotorOutput::write(FlightMixer::testModeOutputs(roll, throttleEsc));
    return;
  }

  // 平衡辅助：
  // 给所有电机相同油门，并通过 ESC_TRIM1..4 调整平衡。
  if (balanceAssistEnabled) {
    MotorOutput::write(FlightMixer::balanceOutputs(throttleEsc));
    return;
  }

  // 正常飞行流程：
  // X 架混控 + 可选自动平衡修正。
  // 仅当飞手开启自动平衡、IMU 初始化成功并且传感器开关仍处于开启时，才允许自动平衡。
  bool autoLevelActive = autoLevelEnabled && sensorsEnabled && ImuManager::isReady();
  MotorOutputs outputs = FlightMixer::flightOutputs(
      roll, pitch, yaw, throttleEsc, autoLevelActive, ImuManager::roll(), ImuManager::pitch());
  MotorOutput::write(outputs);
}
