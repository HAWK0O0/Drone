#pragma once

#include <Arduino.h>

// ============================================================================
// 飞行控制数学库
// 包含约束和限制函数，保证数值在安全范围内
// ============================================================================
namespace ControllerMath {

// 约束16位无符号整数的范围
// value: 要约束的值
// low: 最小值
// high: 最大值
// 返回: 约束后的值
inline uint16_t constrainU16(int value, uint16_t low, uint16_t high) {
  if (value < static_cast<int>(low)) return low;
  if (value > static_cast<int>(high)) return high;
  return static_cast<uint16_t>(value);
}

// 约束整数范围
// value: 要约束的值
// low: 最小值
// high: 最大值
// 返回: 约束后的值
inline int constrainInt(int value, int low, int high) {
  if (value < low) return low;
  if (value > high) return high;
  return value;
}

}  // namespace ControllerMath
