#include "ESC/motor_mixer.h"

#include <math.h>

#include "Control/controller_math.h"
#include "INPUT/drone_config.h"

namespace {

// Motor mixing signs for X-quad configuration
// علامات خلط المحركات لتكوين X-Quad
// 中文: X型四旋翼混控符号
// Motor order: M1 front-left, M2 front-right, M3 rear-left, M4 rear-right
// ترتيب المحركات: M1 أمامي يسار، M2 أمامي يمين، M3 خلفي يسار، M4 خلفي يمين
// 中文: 电机顺序：M1左前，M2右前，M3左后，M4右后
constexpr int kPitchMixSign[4] = {1, 1, -1, -1};   // M1/M2 +pitch, M3/M4 -pitch
constexpr int kRollMixSign[4] = {1, -1, 1, -1};    // M1/M3 +roll, M2/M4 -roll
constexpr int kYawMixSign[4] = {1, -1, -1, 1};     // M1/M4 +yaw, M2/M3 -yaw

// Previous motor outputs for slew rate limiting
// مخارج المحركات السابقة للحد من معدل التغيير
// 中文: 用于变化率限制的先前电机输出
uint16_t g_last_motor_us[4] = {
    DroneConfig::kEscMinUs,
    DroneConfig::kEscMinUs,
    DroneConfig::kEscMinUs,
    DroneConfig::kEscMinUs,
};

// Apply slew rate limiting to motor output changes
// تطبيق الحد من معدل التغيير على تغييرات خرج المحرك
// 中文: 对电机输出变化应用变化率限制
uint16_t ApplySlew(uint8_t index, int target_us) {
  const int lower = static_cast<int>(g_last_motor_us[index]) - DroneConfig::kMotorOutputSlewUs;
  const int upper = static_cast<int>(g_last_motor_us[index]) + DroneConfig::kMotorOutputSlewUs;
  const int slewed = ControllerMath::ClampInt(target_us, lower, upper);
  return ControllerMath::ClampUs(slewed, DroneConfig::kEscMinUs, DroneConfig::kEscMaxUs);
}

}  // namespace

// Reset motor mixer to idle state
// إعادة تعيين مكسر المحركات إلى حالة الخمول
// 中文: 将电机混控重置为怠速状态
void MotorMixer::reset() {
  for (uint8_t i = 0; i < 4; ++i) {
    g_last_motor_us[i] = DroneConfig::kEscMinUs;
  }
}

// Main mixing function - combines throttle, pitch, roll, and yaw into motor outputs
// دالة الخلط الرئيسية - تخلط الخنق والميل والدوران والانعراج لإخراج المحركات
// 中文: 主混控函数 - 将油门、俯仰、横滚和偏航混合为电机输出
void MotorMixer::mix(uint16_t base_throttle_us, float pitch_mix_us, float roll_mix_us,
                     float yaw_mix_us, const FlightSettings& settings, uint16_t motor_us[4]) {
  const uint16_t armed_min_us =
      settings.motor_idle_us > DroneConfig::kEscMinUs ? settings.motor_idle_us
                                                      : DroneConfig::kEscMinUs;

  // Calculate trim scaling factor based on throttle headroom
  // حساب عامل مقياس الترميم بناءً على مساحة الرأس للخنق
  // 中文: 根据油门余量计算微调配比系数
  const float trim_scale =
      ControllerMath::Clamp(static_cast<float>(DroneConfig::kEscMaxUs - base_throttle_us) /
                                static_cast<float>(DroneConfig::kEscMaxUs - DroneConfig::kEscMinUs),
                            0.0f, 1.0f);

  // Mix controls for each motor with weight and trim
  // خلط التحكمات لكل محرك مع الوزن والترميم
  // 中文: 为每个电机混合控制量，考虑重量系数和微调
  for (uint8_t i = 0; i < 4; ++i) {
    const float control_mix =
        pitch_mix_us * kPitchMixSign[i] + roll_mix_us * kRollMixSign[i] +
        yaw_mix_us * kYawMixSign[i];
    const float weighted_mix = control_mix * settings.motor_weight[i];
    const float faded_trim = static_cast<float>(settings.motor_trim_us[i]) * trim_scale;
    const int mixed_target =
      static_cast<int>(lroundf(static_cast<float>(base_throttle_us) + weighted_mix + faded_trim));
    const int target = ControllerMath::ClampInt(mixed_target, armed_min_us, DroneConfig::kEscMaxUs);
    motor_us[i] = ApplySlew(i, target);
    g_last_motor_us[i] = motor_us[i];
  }
}
