#pragma once

// Motor Mixer Module
// وحدة مكسر المحركات
// 中文: 电机混控模块
// Mixes throttle, pitch, roll, and yaw commands into 4 motor outputs
// تخلط أوامر الخنق، الميل، الدوران والانعراج لإخراج 4 محركات
// 中文: 将油门、俯仰、横滚和偏航命令混合到4个电机输出
// This module is responsible for mixing the drone's flight commands into individual motor outputs.
// This module is used to control the drone's movement by adjusting the speed of each motor.

// Motor Output Module
// وحدة خرج المحركات
// 中文: 电机输出模块
// Controls physical motor PWM output signals
// تتحكم في إشارات PWM الفيزيائية للمحركات
// 中文: 控制物理电机PWM输出信号

#include "INPUT/drone_types.h"

namespace MotorMixer {

void reset();
void mix(uint16_t base_throttle_us, float pitch_mix_us, float roll_mix_us, float yaw_mix_us,
         const FlightSettings& settings, uint16_t motor_us[4]);

}  // namespace MotorMixer
