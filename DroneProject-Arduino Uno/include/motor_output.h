#pragma once

#include "flight_mixer.h"

// ============================================================================
// 电机输出管理器
// 控制4个电机，输出PWM信号给电调(ESC)
// ============================================================================
namespace MotorOutput {

// 初始化电机输出引脚和PWM定时器
void begin();

// 停止所有电机
void writeStopAll();

// 输出电机脉宽值(微秒)
// m1-m4: 四个电机的脉宽值 (1000-2000 μs)
void write(uint16_t m1, uint16_t m2, uint16_t m3, uint16_t m4);

// 输出电机结构体
// 使用MotorOutputs结构体设置电机值
void write(const MotorOutputs &outputs);

}  // namespace MotorOutput
