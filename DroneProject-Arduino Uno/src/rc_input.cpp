#include <Arduino.h>
#include "controller_config.h"
#include "rc_input.h"

namespace {

// ============================================================================
// 存储遥控器每个通道的读数变量
// ============================================================================
// 每个通道最新的脉冲宽度(微秒) - 使用通道索引 (1..8)
volatile uint16_t rcUs[9];

// 每个物理引脚的上升沿时间戳
volatile uint32_t riseTimeUs[14];

// 每个通道最后一次有效更新的时间戳
volatile uint32_t lastUpdateUs[9];

// 端口最后状态存储(用于检测变化)
volatile uint8_t lastPINB = 0;
volatile uint8_t lastPIND = 0;

// ============================================================================
// 辅助函数 - 引脚和通道之间的转换
// ============================================================================

// 检查引脚是否有效 (0-13)
inline bool isValidPin(uint8_t pin) {
  return pin <= 13;
}

// ============================================================================
// 引脚到通道的映射 - 新的接线方案
// ============================================================================
// 映射: CH1->D2, CH2->D4, CH3->D5, CH4->D6, CH5->D7, CH6->D8
inline uint8_t pinToChannel(uint8_t pin) {
  if (pin == Config::PIN_CH1) return 1;   // 滚转(Roll)
  if (pin == Config::PIN_CH2) return 2;   // 俯仰(Pitch)
  if (pin == Config::PIN_CH3) return 3;   // 油门(Throttle)
  if (pin == Config::PIN_CH4) return 4;   // 偏航(Yaw)
  if (pin == Config::PIN_CH5) return 5;   // 模式开关
  if (pin == Config::PIN_CH6) return 6;   // 武装开关
  if (pin == Config::PIN_CH7) return 7;   // 额外通道
  if (pin == Config::PIN_CH8) return 8;   // 额外通道
  return 0;  // 错误
}

// 配置输入引脚
inline void configureInputPin(uint8_t pin) {
  if (!isValidPin(pin)) return;
  pinMode(pin, INPUT);  // 输入模式
}

// 在端口变化中断中启用该引脚
inline void enablePinChangeForPin(uint8_t pin, bool &usePortB, bool &usePortD) {
  if (!isValidPin(pin)) return;

  // D0..D7 => PCMSK2 比特 0..7
  if (pin <= 7) {
    PCMSK2 |= (1 << pin);
    usePortD = true;
    return;
  }

  // D8..D13 => PCMSK0 比特 0..5
  if (pin <= 13) {
    PCMSK0 |= (1 << (pin - 8));
    usePortB = true;
  }
}

// ============================================================================
// 从端口变化边沿测量脉冲宽度
// ============================================================================
// 仅接受有效的接收机脉冲范围 (900..2200 微秒)
inline void handlePinChange(uint8_t pin, bool isHigh, uint32_t nowUs) {
  uint8_t channel = pinToChannel(pin);
  if (channel == 0) return;  // 无效通道

  if (isHigh) {
    // 记录上升沿时间
    riseTimeUs[pin] = nowUs;
    return;
  }

  // 计算脉冲宽度
  uint32_t pulseWidth = nowUs - riseTimeUs[pin];
  
  // 检查脉冲是否在有效范围内
  if (pulseWidth >= 900 && pulseWidth <= 2200) {
    rcUs[channel] = static_cast<uint16_t>(pulseWidth);
    lastUpdateUs[channel] = nowUs;
  }
}

}  // 命名空间 (namespace)

// ============================================================================
// 配置 RC 输入引脚并启用端口变化中断 (PCINT)
// ============================================================================
void RcInput::begin() {
  configureInputPin(Config::PIN_CH1);
  configureInputPin(Config::PIN_CH2);
  configureInputPin(Config::PIN_CH3);
  configureInputPin(Config::PIN_CH4);
  configureInputPin(Config::PIN_CH5);
  configureInputPin(Config::PIN_CH6);
  configureInputPin(Config::PIN_CH7);
  configureInputPin(Config::PIN_CH8);

  uint32_t nowUs = micros();
  noInterrupts();
  for (uint8_t channel = 1; channel <= 8; channel++) {
    rcUs[channel] = Config::RC_MID;
    lastUpdateUs[channel] = nowUs;
  }
  rcUs[3] = Config::RC_MIN; // 油门通道初始设为最小
  interrupts();

  lastPINB = PINB;
  lastPIND = PIND;

  PCMSK0 = 0;
  PCMSK2 = 0;
  bool usePortB = false;
  bool usePortD = false;

  enablePinChangeForPin(Config::PIN_CH1, usePortB, usePortD);
  enablePinChangeForPin(Config::PIN_CH2, usePortB, usePortD);
  enablePinChangeForPin(Config::PIN_CH3, usePortB, usePortD);
  enablePinChangeForPin(Config::PIN_CH4, usePortB, usePortD);
  enablePinChangeForPin(Config::PIN_CH5, usePortB, usePortD);
  enablePinChangeForPin(Config::PIN_CH6, usePortB, usePortD);
  enablePinChangeForPin(Config::PIN_CH7, usePortB, usePortD);
  enablePinChangeForPin(Config::PIN_CH8, usePortB, usePortD);

  if (usePortB) {
    PCICR |= (1 << PCIE0);
  } else {
    PCICR &= ~(1 << PCIE0);
  }

  if (usePortD) {
    PCICR |= (1 << PCIE2);
  } else {
    PCICR &= ~(1 << PCIE2);
  }
}

// 线程安全的通道读取函数（带回退值）
uint16_t RcInput::read(uint8_t channel, uint16_t fallback) {
  if (channel == 0 || channel > 8) return fallback;

  uint16_t value;
  noInterrupts(); // 进入临界区，防止读取时被中断修改
  value = rcUs[channel];
  interrupts();

  if (value < 900 || value > 2200) return fallback;
  return value;
}

// 关键控制通道 (1..4) 的数据新鲜度检查（用于失控保护）
bool RcInput::frameFresh() {
  uint32_t nowUs = micros();
  for (uint8_t channel = 1; channel <= 4; channel++) {
    uint32_t t;
    noInterrupts();
    t = lastUpdateUs[channel];
    interrupts();
    if ((nowUs - t) > Config::FAILSAFE_US) return false;
  }
  return true;
}

// 单个通道的数据新鲜度检查
bool RcInput::channelFresh(uint8_t channel) {
  if (channel == 0 || channel > 8) return false;

  uint32_t nowUs = micros();
  uint32_t t;
  noInterrupts();
  t = lastUpdateUs[channel];
  interrupts();
  return (nowUs - t) <= Config::FAILSAFE_US;
}

// 将 PWM 开关值转换为逻辑开关状态 (开/关)
// 中间区域保持当前状态，以防止开关抖动或歧义。
bool RcInput::switchFromValue(uint16_t value, bool currentState) {
  if (value <= Config::SW_OFF) return false;
  if (value >= Config::SW_ON) return true;
  return currentState;
}

// D8..D13 (PORTB) 的引脚变化中断处理程序 (ISR)
ISR(PCINT0_vect) {
  uint32_t nowUs = micros();
  uint8_t current = PINB;
  uint8_t changed = (current ^ lastPINB) & PCMSK0;
  lastPINB = current;

  for (uint8_t bit = 0; bit <= 5; bit++) {
    if (changed & (1 << bit)) {
      uint8_t pin = static_cast<uint8_t>(8 + bit);
      bool isHigh = (current & (1 << bit)) != 0;
      handlePinChange(pin, isHigh, nowUs);
    }
  }
}

// D0..D7 (PORTD) 的引脚变化中断处理程序 (ISR)
ISR(PCINT2_vect) {
  uint32_t nowUs = micros();
  uint8_t current = PIND;
  uint8_t changed = (current ^ lastPIND) & PCMSK2;
  lastPIND = current;

  for (uint8_t bit = 0; bit <= 7; bit++) {
    if (changed & (1 << bit)) {
      uint8_t pin = bit;
      bool isHigh = (current & (1 << bit)) != 0;
      handlePinChange(pin, isHigh, nowUs);
    }
  }
}