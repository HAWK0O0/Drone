#pragma once

#include <Arduino.h>

// ============================================================================
// RC遥控接收器管理器
// 读取来自遥控器的信号 - 油门、翻滚、俯仰、偏航等
// ============================================================================
namespace RcInput {

// 初始化遥控接收器
// 配置中断引脚和定时器
void begin();

// 读取指定通道的遥控值(微秒)
// channel: 通道号 (0-7)
// fallback: 如果没有信号返回的默认值
uint16_t read(uint8_t channel, uint16_t fallback);

// 检查是否收到新的遥控帧
bool frameFresh();

// 检查指定通道是否有新数据
bool channelFresh(uint8_t channel);

// 从遥控值判断开关状态
// value: 遥控脉宽值
// currentState: 当前状态
bool switchFromValue(uint16_t value, bool currentState);

}  // namespace RcInput
