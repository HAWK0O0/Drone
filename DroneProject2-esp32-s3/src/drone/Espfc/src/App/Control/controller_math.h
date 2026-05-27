#pragma once

#include <Arduino.h>
#include <math.h>

namespace ControllerMath {

inline float Clamp(float value, float low, float high) {
  if (value < low) return low;
  if (value > high) return high;
  return value;
}

inline int ClampInt(int value, int low, int high) {
  if (value < low) return low;
  if (value > high) return high;
  return value;
}

inline uint16_t ClampUs(int value, uint16_t low, uint16_t high) {
  if (value < static_cast<int>(low)) return low;
  if (value > static_cast<int>(high)) return high;
  return static_cast<uint16_t>(value);
}

inline float NormalizeStick(uint16_t value_us, uint16_t mid_us) {
  return Clamp((static_cast<float>(value_us) - static_cast<float>(mid_us)) / 500.0f,
               -1.0f, 1.0f);
}

inline float NormalizeThrottle(uint16_t value_us, uint16_t min_us, uint16_t max_us) {
  const float span = static_cast<float>(max_us - min_us);
  if (span <= 0.0f) return 0.0f;
  return Clamp((static_cast<float>(value_us) - static_cast<float>(min_us)) / span,
               0.0f, 1.0f);
}

inline float ApplyDeadband(float normalized_value, float deadband) {
  if (deadband <= 0.0f) return normalized_value;
  if (normalized_value > deadband) {
    return (normalized_value - deadband) / (1.0f - deadband);
  }
  if (normalized_value < -deadband) {
    return (normalized_value + deadband) / (1.0f - deadband);
  }
  return 0.0f;
}

inline float ApplyExpo(float normalized_value, float expo) {
  expo = Clamp(expo, 0.0f, 1.0f);
  const float cubic = normalized_value * normalized_value * normalized_value;
  return normalized_value * (1.0f - expo) + cubic * expo;
}

inline float Lerp(float current, float target, float alpha) {
  alpha = Clamp(alpha, 0.0f, 1.0f);
  return current + (target - current) * alpha;
}

inline float WrapDegrees180(float degrees) {
  while (degrees > 180.0f) degrees -= 360.0f;
  while (degrees < -180.0f) degrees += 360.0f;
  return degrees;
}

inline float DeltaDegrees(float current, float previous) {
  return WrapDegrees180(current - previous);
}

}  // namespace ControllerMath
