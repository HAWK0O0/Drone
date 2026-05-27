#pragma once

#include <Arduino.h>

// ============================================================================
// 电机输出数据结构
// 存储4个电机的PWM脉冲宽度（微秒）
// 典型范围: 1000μs ~ 2000μs
// ============================================================================
struct MotorOutputs {
  uint16_t m1;  // 电机1输出 (前-左)
  uint16_t m2;  // 电机2输出 (前-右)
  uint16_t m3;  // 电机3输出 (后-右)
  uint16_t m4;  // 电机4输出 (后-左)
};

namespace FlightMixer {

uint16_t mapThrottleToEsc(uint16_t throttleUs);
MotorOutputs stopOutputs();
MotorOutputs passthroughOutputs(uint16_t throttleEsc);
MotorOutputs balanceOutputs(uint16_t throttleEsc);
MotorOutputs testModeOutputs(uint16_t rollUs, uint16_t throttleEsc);
MotorOutputs flightOutputs(uint16_t rollUs, uint16_t pitchUs, uint16_t yawUs,
                           uint16_t throttleEsc, bool autoLevelEnabled,
                           float imuRoll, float imuPitch);

}  // namespace FlightMixer
