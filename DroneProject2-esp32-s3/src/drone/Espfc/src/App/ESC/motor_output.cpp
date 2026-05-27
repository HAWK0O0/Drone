#include "ESC/motor_output.h"

#include <Arduino.h>

#include "Control/controller_math.h"
#include "INPUT/drone_config.h"

namespace {

// ESP32-S3 LEDC channels are logical channels, not GPIO pin numbers
// قنوات LEDC لـ ESP32-S3 هي قنوات منطقية وليست أرقام دبابيس GPIO
// 中文: ESP32-S3 LEDC通道是逻辑通道，不是GPIO引脚号
constexpr uint8_t kLedcChannels[4] = {0, 1, 2, 3};
constexpr uint8_t kMinMotorPwmBits = 10;  // Minimum PWM resolution / أقل دقة PWM / 最小PWM分辨率

// Module state variables
// متغيرات حالة الوحدة
// 中文: 模块状态变量
bool g_ready = false;                    // Driver ready flag / علم جاهزية المشغل / 驱动就绪标志
bool g_channel_ready[4] = {false, false, false, false};  // Channel status / حالة القناة / 通道状态
uint16_t g_last_motor_us[4] = {          // Last motor outputs / آخر مخارج المحركات / 上次电机输出
    DroneConfig::kEscMinUs, DroneConfig::kEscMinUs,
    DroneConfig::kEscMinUs, DroneConfig::kEscMinUs,
};
uint8_t g_active_pwm_bits = 0;          // Active PWM bit depth / عمق بت PWM النشط / 有效PWM位深度

// Convert pulse width (microseconds) to LEDC duty cycle
// تحويل عرض النبضة (ميكروثانية) إلى دورة عمل LEDC
// 中文: 将脉宽（微秒）转换为LEDC占空比
uint32_t PulseToDuty(uint16_t pulse_us) {
  const uint8_t duty_bits = g_active_pwm_bits == 0 ? DroneConfig::kMotorPwmBits : g_active_pwm_bits;
  const uint32_t max_duty = (1UL << duty_bits) - 1UL;
  const uint32_t period_us = 1000000UL / DroneConfig::kMotorPwmHz;
  return (static_cast<uint32_t>(pulse_us) * max_duty) / period_us;
}

// Setup LEDC channel with automatic bit depth fallback
// إعداد قناة LEDC مع تراجع تلقائي لعمق البت
// 中文: 设置LEDC通道，自动回退位深度
uint32_t SetupChannel(uint8_t channel) {
  for (int bits = DroneConfig::kMotorPwmBits; bits >= kMinMotorPwmBits; --bits) {
    const uint32_t configured_hz =
        ledcSetup(channel, DroneConfig::kMotorPwmHz, static_cast<uint8_t>(bits));
    if (configured_hz > 0) {
      g_active_pwm_bits = static_cast<uint8_t>(bits);
      return configured_hz;
    }
  }
  return 0;
}

}  // namespace

// Initialize motor output hardware
// تهيئة عتاد خرج المحركات
// 中文: 初始化电机输出硬件
void MotorOutput::begin() {
  g_ready = true;
  g_active_pwm_bits = 0;
  
  // Configure each motor channel
  // إعداد كل قناة محرك
  // 中文: 配置每个电机通道
  for (uint8_t i = 0; i < 4; ++i) {
    pinMode(DroneConfig::kMotorPins[i], OUTPUT);
    digitalWrite(DroneConfig::kMotorPins[i], LOW);

    const double configured_hz = SetupChannel(kLedcChannels[i]);
    g_channel_ready[i] = configured_hz > 0.0;
    g_ready = g_ready && g_channel_ready[i];
    if (!g_channel_ready[i]) {
      Serial.printf("ESC[%u] setup failed on GPIO%u at %u Hz\n", i,
                    DroneConfig::kMotorPins[i], DroneConfig::kMotorPwmHz);
      continue;
    }
    ledcAttachPin(DroneConfig::kMotorPins[i], kLedcChannels[i]);
    Serial.printf("ESC[%u] GPIO%u -> LEDC%u at %.0f Hz, %u bits\n", i,
                  DroneConfig::kMotorPins[i], kLedcChannels[i], configured_hz,
                  g_active_pwm_bits);
  }
  Serial.printf("ESC driver ready: %s\n", g_ready ? "YES" : "NO");
  stopAll();
}

// Check if motor driver is ready
// التحقق مما إذا كان مشغل المحركات جاهزاً
// 中文: 检查电机驱动是否就绪
bool MotorOutput::ready() {
  return g_ready;
}

// Stop all motors (set to minimum pulse)
// إيقاف جميع المحركات (تعيين إلى أقل نبضة)
// 中文: 停止所有电机（设为最小脉宽）
void MotorOutput::stopAll() {
  const uint16_t stopped[4] = {DroneConfig::kEscMinUs, DroneConfig::kEscMinUs,
                               DroneConfig::kEscMinUs, DroneConfig::kEscMinUs};
  write(stopped);
}

// Write motor pulse widths to hardware
// كتابة عرض نبضات المحركات إلى العتاد
// 中文: 将电机脉宽写入硬件
void MotorOutput::write(const uint16_t motor_us[4]) {
  for (uint8_t i = 0; i < 4; ++i) {
    const uint16_t clamped =
        ControllerMath::ClampUs(motor_us[i], DroneConfig::kEscMinUs, DroneConfig::kEscMaxUs);
    g_last_motor_us[i] = g_channel_ready[i] ? clamped : DroneConfig::kEscMinUs;
    if (!g_channel_ready[i]) continue;
    ledcWrite(kLedcChannels[i], PulseToDuty(clamped));
  }
}

// Get current motor output snapshot
// الحصول على لقطة خرج المحركات الحالية
// 中文: 获取当前电机输出快照
void MotorOutput::snapshot(uint16_t motor_us[4]) {
  for (uint8_t i = 0; i < 4; ++i) {
    motor_us[i] = g_last_motor_us[i];
  }
}
