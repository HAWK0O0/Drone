#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "controller_config.h"
#include "motor_output.h"

namespace {

// ============================================================================
// 电机 PWM 同步输出 - 四个电机均位于 PORTB 端口
// ============================================================================
// ESC1=D12 (PB4), ESC2=D9 (PB1), ESC3=D10 (PB2), ESC4=D11 (PB3)
// 所有引脚集中在同一端口，通过寄存器同步写入实现真正的零延迟 (Zero-latency)。
constexpr uint8_t ESC1_MASK_B = _BV(PB4);   // D12 - 电机 1
constexpr uint8_t ESC2_MASK_B = _BV(PB1);   // D9  - 电机 2
constexpr uint8_t ESC3_MASK_B = _BV(PB2);   // D10 - 电机 3
constexpr uint8_t ESC4_MASK_B = _BV(PB3);   // D11 - 电机 4
constexpr uint8_t ESC_ALL_MASK_B = ESC1_MASK_B | ESC2_MASK_B | ESC3_MASK_B | ESC4_MASK_B;

// ============================================================================
// 定时器参数与脉冲计算
// ============================================================================
constexpr uint16_t TICKS_PER_US = 2; // Timer1 @ 8分频，0.5us/tick
constexpr uint16_t FRAME_TICKS = Config::ESC_FRAME_US * TICKS_PER_US;

// 脉冲命令变量 (volatile 确保主循环与 ISR 之间的原子同步)
volatile uint16_t pulseTicks1 = Config::ESC_STOP * TICKS_PER_US; 
volatile uint16_t pulseTicks2 = Config::ESC_STOP * TICKS_PER_US; 
volatile uint16_t pulseTicks3 = Config::ESC_STOP * TICKS_PER_US; 
volatile uint16_t pulseTicks4 = Config::ESC_STOP * TICKS_PER_US; 

// ============================================================================
// 事件管理系统 (Event Management)
// ============================================================================
struct EdgeEvent {
  uint16_t tick;      // 触发时刻 (Ticks)
  uint8_t clrMaskB;   // 此时刻需拉低的引脚掩码
};

EdgeEvent events[4];
uint8_t eventCount = 0;
uint8_t eventIndex = 0;
uint16_t lastEventTick = 0;

// 限幅函数：确保输出信号不超出电调行程
inline uint16_t clampPulseUs(uint16_t us) {
  if (us < Config::ESC_STOP) return Config::ESC_STOP;
  if (us > Config::ESC_MAX) return Config::ESC_MAX;
  return us;
}

// 调度下一次比较匹配中断
inline void scheduleDeltaTicks(uint16_t deltaTicks) {
  if (deltaTicks == 0) deltaTicks = 1;
  OCR1A = static_cast<uint16_t>(deltaTicks - 1);
}

// 添加下降沿事件（自动合并相同时间戳的事件）
inline void addClearEvent(uint16_t tick, uint8_t clrMaskB) {
  for (uint8_t i = 0; i < eventCount; i++) {
    if (events[i].tick == tick) {
      events[i].clrMaskB |= clrMaskB;
      return;
    }
  }
  events[eventCount].tick = tick;
  events[eventCount].clrMaskB = clrMaskB;
  eventCount++;
}

// 插入排序：确保事件按时间顺序排列
inline void sortEventsByTick() {
  for (uint8_t i = 1; i < eventCount; i++) {
    EdgeEvent key = events[i];
    int8_t j = static_cast<int8_t>(i) - 1;
    while (j >= 0 && events[j].tick > key.tick) {
      events[j + 1] = events[j];
      j--;
    }
    events[j + 1] = key;
  }
}

// 启动新帧周期并预构建事件表
inline void startFrameAndBuildEvents() {
  PORTB |= ESC_ALL_MASK_B; // 同步拉高所有通道信号

  uint16_t t1 = pulseTicks1;
  uint16_t t2 = pulseTicks2;
  uint16_t t3 = pulseTicks3;
  uint16_t t4 = pulseTicks4;

  eventCount = 0;
  eventIndex = 0;
  lastEventTick = 0;

  addClearEvent(t1, ESC1_MASK_B);
  addClearEvent(t2, ESC2_MASK_B);
  addClearEvent(t3, ESC3_MASK_B);
  addClearEvent(t4, ESC4_MASK_B);
  sortEventsByTick();
}

// 初始化 Timer1 (CTC 模式)
inline void setupTimer1() {
  cli(); 
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  TCCR1B |= (1 << WGM12) | (1 << CS11); // WGM12=CTC, CS11=8分频
  scheduleDeltaTicks(200); 
  TIMSK1 |= (1 << OCIE1A); 
  sei(); 
}

} // 命名空间结束

// ============================================================================
// MotorOutput 类接口实现
// ============================================================================

void MotorOutput::begin() {
  // 设置 PORTB 对应引脚为输出模式
  DDRB |= ESC_ALL_MASK_B;
  PORTB &= (uint8_t)(~ESC_ALL_MASK_B);

  writeStopAll();
  setupTimer1();
}

void MotorOutput::writeStopAll() {
  write(Config::ESC_STOP, Config::ESC_STOP, Config::ESC_STOP, Config::ESC_STOP);
}

void MotorOutput::write(uint16_t m1, uint16_t m2, uint16_t m3, uint16_t m4) {
  uint16_t c1 = clampPulseUs(m1);
  uint16_t c2 = clampPulseUs(m2);
  uint16_t c3 = clampPulseUs(m3);
  uint16_t c4 = clampPulseUs(m4);

  noInterrupts(); // 进入临界区，更新脉冲宽度快照
  pulseTicks1 = c1 * TICKS_PER_US;
  pulseTicks2 = c2 * TICKS_PER_US;
  pulseTicks3 = c3 * TICKS_PER_US;
  pulseTicks4 = c4 * TICKS_PER_US;
  interrupts();
}

void MotorOutput::write(const MotorOutputs &outputs) {
  write(outputs.m1, outputs.m2, outputs.m3, outputs.m4);
}

// ============================================================================
// Timer1 比较匹配中断服务程序 (ISR)
// ============================================================================
ISR(TIMER1_COMPA_vect) {
  // 帧起始：拉高信号并调度第一个下降沿事件
  if (eventCount == 0) {
    startFrameAndBuildEvents();
    scheduleDeltaTicks(events[0].tick);
    return;
  }

  // 执行预定的引脚拉低事件
  PORTB &= (uint8_t)(~events[eventIndex].clrMaskB);
  lastEventTick = events[eventIndex].tick;
  eventIndex++;

  if (eventIndex < eventCount) {
    // 处理下一个下降沿
    uint16_t delta = events[eventIndex].tick - lastEventTick;
    scheduleDeltaTicks(delta);
    return;
  }

  // 帧完成：等待剩余时间后触发下一帧
  uint16_t gap = FRAME_TICKS - lastEventTick;
  eventCount = 0;
  eventIndex = 0;
  lastEventTick = 0;
  scheduleDeltaTicks(gap);
}