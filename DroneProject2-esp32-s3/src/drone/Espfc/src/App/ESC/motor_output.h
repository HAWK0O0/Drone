#pragma once

// Motor Output Module
// وحدة خرج المحركات
// 中文: 电机输出模块
// Controls physical motor PWM output signals
// تتحكم في إشارات PWM الفيزيائية للمحركات
// 中文: 控制物理电机PWM输出信号

#include "INPUT/drone_types.h"

namespace MotorOutput {

void begin();
bool ready();
void stopAll();
void write(const uint16_t motor_us[4]);
void snapshot(uint16_t motor_us[4]);

}  // namespace MotorOutput
