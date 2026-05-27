#include "INPUT/settings_store.h"
#include <Preferences.h>
#include <math.h>
#include <string.h>
#include "Control/controller_math.h"
#include "INPUT/drone_config.h"

namespace {
// Flash storage identifiers
// معرفات تخزين الفلاش
// 中文: 闪存标识符
constexpr char kPreferencesNamespace[] = "dronefc";
constexpr char kSettingsKey[] = "settings";

// Legacy settings structure for migration
// هيكل إعدادات قديم للترحيل
// 中文: 用于迁移的旧设置结构
struct LegacyFlightSettingsV3 {
  uint32_t version;
  PidTuning roll_pid;
  PidTuning pitch_pid;
  float max_angle_deg;
  float rc_expo;
  float manual_mix_us;
  float yaw_mix_us;
  uint16_t motor_idle_us;
  float motor_weight[4];
  int16_t motor_trim_us[4];
  float level_roll_offset_deg;
  float level_pitch_offset_deg;
  char ap_ssid[33];
  char ap_password[65];
};

struct LegacyFlightSettingsV4 {
  uint32_t version;
  PidTuning roll_pid;
  PidTuning pitch_pid;
  float max_angle_deg;
  float rc_expo;
  float manual_mix_us;
  float yaw_mix_us;
  uint16_t motor_idle_us;
  float motor_weight[4];
  int16_t motor_trim_us[4];
  float level_roll_offset_deg;
  float level_pitch_offset_deg;
  char ap_ssid[33];
  char ap_password[65];
  bool sta_enabled;
  char sta_ssid[33];
  char sta_password[65];
};

bool IsFinite(float value) {
  return isfinite(value);
}

void CopyDefaultString(const char* source, char* destination, size_t length) {
  if (length == 0) return;
  strncpy(destination, source, length - 1);
  destination[length - 1] = '\0';
}

}  // namespace

FlightSettings SettingsStore::Defaults() {
  FlightSettings settings{};
  settings.version = DroneConfig::kSettingsVersion;
  settings.roll_pid = {1.45f, 0.030f, 8.0f, 100.0f, 260.0f};
  settings.pitch_pid = {1.45f, 0.030f, 8.0f, 100.0f, 260.0f};
  settings.yaw_pid = {2.2f, 0.020f, 0.0f, 80.0f, 170.0f};
  settings.max_angle_deg = 18.0f;
  settings.rc_expo = 0.30f;
  settings.manual_mix_us = 115.0f;
  settings.yaw_mix_us = 80.0f;
  settings.motor_idle_us = 1110;
  for (size_t i = 0; i < 4; ++i) {
    settings.motor_weight[i] = 1.0f;
    settings.motor_trim_us[i] = 0;
  }
  settings.level_roll_offset_deg = 0.0f;
  settings.level_pitch_offset_deg = 0.0f;
  CopyDefaultString(DroneConfig::kDefaultApSsid, settings.ap_ssid, sizeof(settings.ap_ssid));
  CopyDefaultString(DroneConfig::kDefaultApPassword, settings.ap_password,
                    sizeof(settings.ap_password));
  settings.sta_enabled = DroneConfig::kDefaultStaEnabled;
  CopyDefaultString(DroneConfig::kDefaultStaSsid, settings.sta_ssid, sizeof(settings.sta_ssid));
  CopyDefaultString(DroneConfig::kDefaultStaPassword, settings.sta_password,
                    sizeof(settings.sta_password));
  return settings;
}

void SettingsStore::Sanitize(FlightSettings* settings) {
  if (settings == nullptr) return;

  FlightSettings defaults = Defaults();
  const uint32_t stored_version = settings->version;
  if (settings->version > DroneConfig::kSettingsVersion) {
    *settings = defaults;
    return;
  }
  if (stored_version < 5) {
    settings->roll_pid = defaults.roll_pid;
    settings->pitch_pid = defaults.pitch_pid;
    settings->yaw_pid = defaults.yaw_pid;
    settings->max_angle_deg = defaults.max_angle_deg;
    settings->rc_expo = defaults.rc_expo;
    settings->manual_mix_us = defaults.manual_mix_us;
    settings->yaw_mix_us = defaults.yaw_mix_us;
    settings->motor_idle_us = defaults.motor_idle_us;
  }
  if (settings->version < DroneConfig::kSettingsVersion) {
    settings->version = DroneConfig::kSettingsVersion;
  }

  auto sanitize_pid = [](PidTuning* pid, const PidTuning& fallback) {
    if (!IsFinite(pid->kp)) pid->kp = fallback.kp;
    if (!IsFinite(pid->ki)) pid->ki = fallback.ki;
    if (!IsFinite(pid->kd)) pid->kd = fallback.kd;
    if (!IsFinite(pid->integral_limit)) pid->integral_limit = fallback.integral_limit;
    if (!IsFinite(pid->output_limit)) pid->output_limit = fallback.output_limit;
    pid->kp = ControllerMath::Clamp(pid->kp, 0.1f, 20.0f);
    pid->ki = ControllerMath::Clamp(pid->ki, 0.0f, 2.0f);
    pid->kd = ControllerMath::Clamp(pid->kd, 0.0f, 20.0f);
    pid->integral_limit = ControllerMath::Clamp(pid->integral_limit, 0.0f, 300.0f);
    pid->output_limit = ControllerMath::Clamp(pid->output_limit, 30.0f, 500.0f);
  };

  sanitize_pid(&settings->roll_pid, defaults.roll_pid);
  sanitize_pid(&settings->pitch_pid, defaults.pitch_pid);
  sanitize_pid(&settings->yaw_pid, defaults.yaw_pid);

  if (!IsFinite(settings->max_angle_deg)) settings->max_angle_deg = defaults.max_angle_deg;
  if (!IsFinite(settings->rc_expo)) settings->rc_expo = defaults.rc_expo;
  if (!IsFinite(settings->manual_mix_us)) settings->manual_mix_us = defaults.manual_mix_us;
  if (!IsFinite(settings->yaw_mix_us)) settings->yaw_mix_us = defaults.yaw_mix_us;
  if (!IsFinite(settings->level_roll_offset_deg)) {
    settings->level_roll_offset_deg = defaults.level_roll_offset_deg;
  }
  if (!IsFinite(settings->level_pitch_offset_deg)) {
    settings->level_pitch_offset_deg = defaults.level_pitch_offset_deg;
  }

  settings->max_angle_deg = ControllerMath::Clamp(settings->max_angle_deg, 10.0f, 55.0f);
  settings->rc_expo = ControllerMath::Clamp(settings->rc_expo, 0.0f, 0.85f);
  settings->manual_mix_us = ControllerMath::Clamp(settings->manual_mix_us, 60.0f, 350.0f);
  settings->yaw_mix_us = ControllerMath::Clamp(settings->yaw_mix_us, 40.0f, 260.0f);
  settings->motor_idle_us =
      ControllerMath::ClampUs(settings->motor_idle_us, DroneConfig::kEscMinUs,
                              static_cast<uint16_t>(DroneConfig::kEscMinUs + 120));
  settings->level_roll_offset_deg =
      ControllerMath::Clamp(settings->level_roll_offset_deg, -25.0f, 25.0f);
  settings->level_pitch_offset_deg =
      ControllerMath::Clamp(settings->level_pitch_offset_deg, -25.0f, 25.0f);

  for (size_t i = 0; i < 4; ++i) {
    if (!IsFinite(settings->motor_weight[i])) settings->motor_weight[i] = 1.0f;
    settings->motor_weight[i] = ControllerMath::Clamp(
        settings->motor_weight[i], DroneConfig::kMotorWeightMin, DroneConfig::kMotorWeightMax);
    settings->motor_trim_us[i] = ControllerMath::ClampInt(
        settings->motor_trim_us[i], DroneConfig::kMotorTrimMinUs, DroneConfig::kMotorTrimMaxUs);
  }

  if (settings->ap_ssid[0] == '\0') {
    CopyDefaultString(defaults.ap_ssid, settings->ap_ssid, sizeof(settings->ap_ssid));
  }
  if (strlen(settings->ap_password) < 8) {
    CopyDefaultString(defaults.ap_password, settings->ap_password, sizeof(settings->ap_password));
  }
  settings->ap_ssid[sizeof(settings->ap_ssid) - 1] = '\0';
  settings->ap_password[sizeof(settings->ap_password) - 1] = '\0';

  settings->sta_ssid[sizeof(settings->sta_ssid) - 1] = '\0';
  settings->sta_password[sizeof(settings->sta_password) - 1] = '\0';
  
  // Migration logic: always enable STA with default values if empty or from old version
  if (settings->sta_ssid[0] == '\0' || !settings->sta_enabled) {
    CopyDefaultString(defaults.sta_ssid, settings->sta_ssid, sizeof(settings->sta_ssid));
    CopyDefaultString(defaults.sta_password, settings->sta_password, sizeof(settings->sta_password));
    settings->sta_enabled = true; // Force enable for new migration
  }
  if (settings->sta_password[0] == '\0' && defaults.sta_password[0] != '\0') {
    CopyDefaultString(defaults.sta_password, settings->sta_password,
                      sizeof(settings->sta_password));
    settings->sta_enabled = true; // Enable when password is set
  }
  if (settings->sta_enabled && strlen(settings->sta_password) > 0 &&
      strlen(settings->sta_password) < 8) {
    if (strlen(defaults.sta_password) >= 8) {
      CopyDefaultString(defaults.sta_password, settings->sta_password,
                        sizeof(settings->sta_password));
    } else {
      settings->sta_enabled = false;
      settings->sta_password[0] = '\0';
    }
  }
  if (settings->sta_ssid[0] == '\0') {
    settings->sta_enabled = false;
    settings->sta_password[0] = '\0';
  }
}

FlightSettings SettingsStore::Load() {
  FlightSettings settings = Defaults();

  Preferences preferences;
  if (!preferences.begin(kPreferencesNamespace, true)) {
    return settings;
  }

  const size_t stored_length = preferences.getBytesLength(kSettingsKey);
  if (stored_length == sizeof(FlightSettings)) {
    preferences.getBytes(kSettingsKey, &settings, sizeof(FlightSettings));
  } else if (stored_length == sizeof(LegacyFlightSettingsV4)) {
    LegacyFlightSettingsV4 legacy{};
    preferences.getBytes(kSettingsKey, &legacy, sizeof(LegacyFlightSettingsV4));
    settings.version = DroneConfig::kSettingsVersion;
    settings.roll_pid = legacy.roll_pid;
    settings.pitch_pid = legacy.pitch_pid;
    settings.yaw_pid = Defaults().yaw_pid;
    settings.max_angle_deg = legacy.max_angle_deg;
    settings.rc_expo = legacy.rc_expo;
    settings.manual_mix_us = legacy.manual_mix_us;
    settings.yaw_mix_us = legacy.yaw_mix_us;
    settings.motor_idle_us = legacy.motor_idle_us;
    memcpy(settings.motor_weight, legacy.motor_weight, sizeof(settings.motor_weight));
    memcpy(settings.motor_trim_us, legacy.motor_trim_us, sizeof(settings.motor_trim_us));
    settings.level_roll_offset_deg = legacy.level_roll_offset_deg;
    settings.level_pitch_offset_deg = legacy.level_pitch_offset_deg;
    memcpy(settings.ap_ssid, legacy.ap_ssid, sizeof(settings.ap_ssid));
    memcpy(settings.ap_password, legacy.ap_password, sizeof(settings.ap_password));
    settings.sta_enabled = legacy.sta_enabled;
    memcpy(settings.sta_ssid, legacy.sta_ssid, sizeof(settings.sta_ssid));
    memcpy(settings.sta_password, legacy.sta_password, sizeof(settings.sta_password));
  } else if (stored_length == sizeof(LegacyFlightSettingsV3)) {
    LegacyFlightSettingsV3 legacy{};
    preferences.getBytes(kSettingsKey, &legacy, sizeof(LegacyFlightSettingsV3));
    settings.version = DroneConfig::kSettingsVersion;
    settings.roll_pid = legacy.roll_pid;
    settings.pitch_pid = legacy.pitch_pid;
    settings.yaw_pid = Defaults().yaw_pid;
    settings.max_angle_deg = legacy.max_angle_deg;
    settings.rc_expo = legacy.rc_expo;
    settings.manual_mix_us = legacy.manual_mix_us;
    settings.yaw_mix_us = legacy.yaw_mix_us;
    settings.motor_idle_us = legacy.motor_idle_us;
    memcpy(settings.motor_weight, legacy.motor_weight, sizeof(settings.motor_weight));
    memcpy(settings.motor_trim_us, legacy.motor_trim_us, sizeof(settings.motor_trim_us));
    settings.level_roll_offset_deg = legacy.level_roll_offset_deg;
    settings.level_pitch_offset_deg = legacy.level_pitch_offset_deg;
    memcpy(settings.ap_ssid, legacy.ap_ssid, sizeof(settings.ap_ssid));
    memcpy(settings.ap_password, legacy.ap_password, sizeof(settings.ap_password));
  }
  preferences.end();

  Sanitize(&settings);
  return settings;
}

bool SettingsStore::Save(const FlightSettings& input) {
  FlightSettings settings = input;
  settings.version = DroneConfig::kSettingsVersion;
  Sanitize(&settings);

  Preferences preferences;
  if (!preferences.begin(kPreferencesNamespace, false)) {
    return false;
  }

  const size_t written = preferences.putBytes(kSettingsKey, &settings, sizeof(FlightSettings));
  preferences.end();
  return written == sizeof(FlightSettings);
}
