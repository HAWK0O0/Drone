#include <Arduino.h>
#include "controller_config.h"
#include "controller_math.h"
#include "flight_mixer.h"

namespace {

// ============================================================================
// ESC命令历史 - 供可选速率限制器使用
// ============================================================================
int last1 = Config::ESC_STOP;
int last2 = Config::ESC_STOP;
int last3 = Config::ESC_STOP;
int last4 = Config::ESC_STOP;
int lastAutoCorrR = 0;
int lastAutoCorrP = 0;

// 将指令映射到特定的电机输出。
// 如果 ESC_START_COMP_ENABLED 为 false，则输出为直接指令加微调。
// 如果为 true，则重新映射指令，以确保每个电机都能可靠地启动旋转。
uint16_t applyMotorCurve(uint16_t commandUs, uint16_t startUs, uint16_t maxUs,
                         int trimUs) {
  int command = static_cast<int>(commandUs) + trimUs;
  command = ControllerMath::constrainInt(command, Config::ESC_STOP, Config::ESC_MAX);

  if (!Config::ESC_START_COMP_ENABLED) {
    return static_cast<uint16_t>(command);
  }

  int start = ControllerMath::constrainInt(static_cast<int>(startUs), Config::ESC_STOP, Config::ESC_MAX);
  int maxValue = ControllerMath::constrainInt(static_cast<int>(maxUs), start, Config::ESC_MAX);
  if (command <= static_cast<int>(Config::ESC_STOP)) {
    return Config::ESC_STOP;
  }

  uint32_t x = static_cast<uint32_t>(command - Config::ESC_STOP);
  uint32_t spanOut = static_cast<uint32_t>(maxValue - start);
  uint32_t mapped = static_cast<uint32_t>(start) +
                    (x * spanOut) / (Config::ESC_MAX - Config::ESC_STOP);

  return ControllerMath::constrainU16(static_cast<int>(mapped), Config::ESC_STOP,
                                      static_cast<uint16_t>(maxValue));
}

// 仅针对 ESC2 (D9) 的启动瞬间提供小幅增压，以同步电机的起步时间。
// 仅在低指令区域应用，不覆盖整个油门范围。
uint16_t applyEsc2SyncBoost(uint16_t commandUs) {
  if (Config::ESC2_SYNC_BOOST_US == 0) return commandUs;
  if (commandUs <= Config::ESC_STOP) return commandUs;
  if (commandUs > Config::ESC2_SYNC_BOOST_MAX_US) return commandUs;

  int boosted = static_cast<int>(commandUs) + static_cast<int>(Config::ESC2_SYNC_BOOST_US);
  return ControllerMath::constrainU16(boosted, Config::ESC_STOP, Config::ESC_MAX);
}

// 在测试模式下，通过滚转摇杆 (Roll stick) 选择激活单个电机。
uint8_t getTestMotorIndex(uint16_t rollUs) {
  if (rollUs < 1250) return 1;
  if (rollUs < 1500) return 2;
  if (rollUs < 1750) return 3;
  return 4;
}

int applyCenteredDeadband(int value, int deadband) {
  if (deadband <= 0) return value;
  if (value > deadband) return value - deadband;
  if (value < -deadband) return value + deadband;
  return 0;
}

int applySlew(int current, int target, int step) {
  if (step <= 0) return target;
  if (target > current + step) return current + step;
  if (target < current - step) return current - step;
  return target;
}

// 选用的电调 (ESC) 输出速率限制器 (Slew Limiter)，用于避免信号剧烈跳变。
void applyRateLimiter(int &e1, int &e2, int &e3, int &e4) {
  if (Config::ESC_STEP > 0) {
    if (e1 > last1 + Config::ESC_STEP) e1 = last1 + Config::ESC_STEP;
    if (e1 < last1 - Config::ESC_STEP) e1 = last1 - Config::ESC_STEP;

    if (e2 > last2 + Config::ESC_STEP) e2 = last2 + Config::ESC_STEP;
    if (e2 < last2 - Config::ESC_STEP) e2 = last2 - Config::ESC_STEP;

    if (e3 > last3 + Config::ESC_STEP) e3 = last3 + Config::ESC_STEP;
    if (e3 < last3 - Config::ESC_STEP) e3 = last3 - Config::ESC_STEP;

    if (e4 > last4 + Config::ESC_STEP) e4 = last4 + Config::ESC_STEP;
    if (e4 < last4 - Config::ESC_STEP) e4 = last4 - Config::ESC_STEP;
  }

  last1 = e1;
  last2 = e2;
  last3 = e3;
  last4 = e4;
}

}  // namespace

// 将原始油门输入（通常直接来自通道 3）映射到电调 (ESC) 预期的安全范围内。
// 由于配置常量默认定义为 [1000..2000]，此函数会有效地将被低于 1000 的值
// 剪切为 1000，将被高于 2000 的值剪切为 2000。通过修改 
// `Config::ESC_STOP` 和 `Config::ESC_MAX` 可以轻松更改该范围。
uint16_t FlightMixer::mapThrottleToEsc(uint16_t throttleUs) {
  return ControllerMath::constrainU16(static_cast<int>(throttleUs), Config::ESC_STOP, Config::ESC_MAX);
}

// 所有电机停止输出。
MotorOutputs FlightMixer::stopOutputs() {
  return {Config::ESC_STOP, Config::ESC_STOP, Config::ESC_STOP, Config::ESC_STOP};
}

// 所有电机使用相同的油门（用于直接直通模式）。
MotorOutputs FlightMixer::passthroughOutputs(uint16_t throttleEsc) {
  int e1 = throttleEsc;
  int e2 = throttleEsc;
  int e3 = throttleEsc;
  int e4 = throttleEsc;
  applyRateLimiter(e1, e2, e3, e4);
  return {static_cast<uint16_t>(e1), static_cast<uint16_t>(e2),
          static_cast<uint16_t>(e3), static_cast<uint16_t>(e4)};
}

// 平衡辅助模式：所有电机使用相同的油门指令，但保持各电机的微调/曲线设置处于激活状态。
MotorOutputs FlightMixer::balanceOutputs(uint16_t throttleEsc) {
  int out1 = applyMotorCurve(throttleEsc, Config::ESC1_START_US, Config::ESC1_MAX_US, Config::ESC_TRIM1);
  int out2 = applyMotorCurve(applyEsc2SyncBoost(throttleEsc),
                             Config::ESC2_START_US, Config::ESC2_MAX_US, Config::ESC_TRIM2);
  int out3 = applyMotorCurve(throttleEsc, Config::ESC3_START_US, Config::ESC3_MAX_US, Config::ESC_TRIM3);
  int out4 = applyMotorCurve(throttleEsc, Config::ESC4_START_US, Config::ESC4_MAX_US, Config::ESC_TRIM4);
  applyRateLimiter(out1, out2, out3, out4);
  return {static_cast<uint16_t>(out1), static_cast<uint16_t>(out2),
          static_cast<uint16_t>(out3), static_cast<uint16_t>(out4)};
}

// 台架测试模式：油门仅应用于选定的单个电机。
MotorOutputs FlightMixer::testModeOutputs(uint16_t rollUs, uint16_t throttleEsc) {
  uint8_t motor = getTestMotorIndex(rollUs);
  uint16_t out1 = Config::ESC_STOP;
  uint16_t out2 = Config::ESC_STOP;
  uint16_t out3 = Config::ESC_STOP;
  uint16_t out4 = Config::ESC_STOP;

  if (motor == 1) out1 = applyMotorCurve(throttleEsc, Config::ESC1_START_US, Config::ESC1_MAX_US, Config::ESC_TRIM1);
  if (motor == 2) out2 = applyMotorCurve(applyEsc2SyncBoost(throttleEsc),
                                         Config::ESC2_START_US, Config::ESC2_MAX_US, Config::ESC_TRIM2);
  if (motor == 3) out3 = applyMotorCurve(throttleEsc, Config::ESC3_START_US, Config::ESC3_MAX_US, Config::ESC_TRIM3);
  if (motor == 4) out4 = applyMotorCurve(throttleEsc, Config::ESC4_START_US, Config::ESC4_MAX_US, Config::ESC_TRIM4);

  return {out1, out2, out3, out4};
}

// 主飞行混控器 (Main flight mixer):
// 1) 将遥控 (RC) 输入转换为滚转/俯仰/偏航的中心偏移量
// 2) 应用全局混控比例
// 3) 在摇杆中心附近对滚转/俯仰应用自动调平校正
// 4) 应用 X 型机架方程
// 5) 对每个电机应用微调/曲线设置
// 6) 应用可选的速率限制器
MotorOutputs FlightMixer::flightOutputs(uint16_t rollUs, uint16_t pitchUs, uint16_t yawUs,
                                        uint16_t throttleEsc, bool autoLevelEnabled,
                                        float imuRoll, float imuPitch) {
  int r = static_cast<int>(rollUs) - static_cast<int>(Config::RC_MID);
  int p = static_cast<int>(pitchUs) - static_cast<int>(Config::RC_MID);
  int y = static_cast<int>(yawUs) - static_cast<int>(Config::RC_MID);

  r = applyCenteredDeadband(r, Config::STICK_DEADBAND_US);
  p = applyCenteredDeadband(p, Config::STICK_DEADBAND_US);
  y = applyCenteredDeadband(y, Config::STICK_DEADBAND_US);

  r = static_cast<int>(r * Config::MIX_SCALE);
  p = static_cast<int>(p * Config::MIX_SCALE);
  y = static_cast<int>(y * Config::MIX_SCALE);

  if (autoLevelEnabled && (throttleEsc > (Config::ESC_STOP + Config::AUTO_MIN_THR))) {
    int targetCorrR = static_cast<int>(-imuRoll * Config::AUTO_KP);
    int targetCorrP = static_cast<int>(-imuPitch * Config::AUTO_KP);
    targetCorrR = ControllerMath::constrainInt(targetCorrR, -Config::AUTO_MAX, Config::AUTO_MAX);
    targetCorrP = ControllerMath::constrainInt(targetCorrP, -Config::AUTO_MAX, Config::AUTO_MAX);

    int correctionRoll = applySlew(lastAutoCorrR, targetCorrR, Config::AUTO_CORR_SLEW_STEP);
    int correctionPitch = applySlew(lastAutoCorrP, targetCorrP, Config::AUTO_CORR_SLEW_STEP);
    lastAutoCorrR = correctionRoll;
    lastAutoCorrP = correctionPitch;

    if (abs(static_cast<int>(rollUs) - static_cast<int>(Config::RC_MID)) < Config::AUTO_DEADBAND_US) {
      r += correctionRoll;
    }
    if (abs(static_cast<int>(pitchUs) - static_cast<int>(Config::RC_MID)) < Config::AUTO_DEADBAND_US) {
      p += correctionPitch;
    }
  } else {
    lastAutoCorrR = 0;
    lastAutoCorrP = 0;
  }

  int e1 = static_cast<int>(throttleEsc) - p + r - y;
  int e2 = static_cast<int>(throttleEsc) + p + r + y;
  int e3 = static_cast<int>(throttleEsc) + p - r - y;
  int e4 = static_cast<int>(throttleEsc) - p - r + y;

  uint16_t cmd1 = static_cast<uint16_t>(
      ControllerMath::constrainInt(e1 + Config::ESC_TRIM1, Config::ESC_STOP, Config::ESC_MAX));
  uint16_t cmd2 = static_cast<uint16_t>(
      ControllerMath::constrainInt(e2 + Config::ESC_TRIM2, Config::ESC_STOP, Config::ESC_MAX));
  uint16_t cmd3 = static_cast<uint16_t>(
      ControllerMath::constrainInt(e3 + Config::ESC_TRIM3, Config::ESC_STOP, Config::ESC_MAX));
  uint16_t cmd4 = static_cast<uint16_t>(
      ControllerMath::constrainInt(e4 + Config::ESC_TRIM4, Config::ESC_STOP, Config::ESC_MAX));

  cmd2 = applyEsc2SyncBoost(cmd2);

  e1 = applyMotorCurve(cmd1, Config::ESC1_START_US, Config::ESC1_MAX_US, 0);
  e2 = applyMotorCurve(cmd2, Config::ESC2_START_US, Config::ESC2_MAX_US, 0);
  e3 = applyMotorCurve(cmd3, Config::ESC3_START_US, Config::ESC3_MAX_US, 0);
  e4 = applyMotorCurve(cmd4, Config::ESC4_START_US, Config::ESC4_MAX_US, 0);

  applyRateLimiter(e1, e2, e3, e4);
  return {static_cast<uint16_t>(e1), static_cast<uint16_t>(e2),
          static_cast<uint16_t>(e3), static_cast<uint16_t>(e4)};
}
