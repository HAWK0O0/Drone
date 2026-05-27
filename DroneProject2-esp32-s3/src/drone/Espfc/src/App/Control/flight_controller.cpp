#include "Control/flight_controller.h"

#include <math.h>
#include <string.h>

#include "AHRS/ahrs_manager.h"
#include "Connect/web_link.h"
#include "Control/controller_math.h"
#include "ESC/motor_mixer.h"
#include "ESC/motor_output.h"
#include "ESC/pid_controller.h"
#include "INPUT/drone_config.h"
#include "INPUT/settings_store.h"
#include "OUTPUT/serial_console.h"
#include "OUTPUT/tft_status.h"
#include "RC/rc_receiver.h"
#include "SENSOR/gps_manager.h"
#include "WIFI/wifi_manager.h"
#include "WIFI/web_ui.h"

namespace {

// Critical section lock for thread-safe state access
// قفل القسم الحرج للوصول الآمن للحالة من عدة مفاتيح
// 中文: 关键段锁，用于线程安全的状态访问
portMUX_TYPE g_state_lock = portMUX_INITIALIZER_UNLOCKED;

// Flight state variables
// متغيرات حالة الطيران
// 中文: 飞行状态变量
FlightSettings g_settings{};           // Current flight settings / الإعدادات الحالية للطيران / 当前飞行设置
TelemetrySnapshot g_telemetry{};       // Current telemetry data / بيانات التليمترية الحالية / 当前遥测数据

// PID controllers for attitude stabilization
// متحكمات PID لتثبيت الوضعية
// 中文: 姿态稳定PID控制器
PidController g_roll_pid;              // Roll axis PID / PID محور الدوران / 横滚轴PID
PidController g_pitch_pid;             // Pitch axis PID / PID محور الميل / 俯仰轴PID
PidController g_yaw_pid;               // Yaw axis PID / PID محور الانعراج / 偏航轴PID

// Arming and timing state
// حالة التسليح والتوقيت
// 中文: 解锁和定时状态
bool g_armed = false;                  // Motors armed flag / علم تسليح المحركات / 电机解锁标志
uint32_t g_last_control_us = 0;        // Last control loop time / وقت آخر حلقة تحكم / 上次控制循环时间
uint32_t g_last_loop_rate_ms = 0;      // Last loop rate check / آخر فحص لمعدل الحلقة / 上次循环速率检查
uint32_t g_last_web_service_ms = 0;    // Last web service slice / آخر شريحة خدمة للويب / 上次网页服务切片
uint32_t g_last_gps_refresh_us = 0;    // Last GPS service slice / آخر شريحة خدمة للـ GPS / 上次 GPS 服务切片
uint32_t g_loop_counter = 0;         // Loop counter for rate calculation / عداد الحلقة لحساب المعدل / 循环计数器
float g_loop_load_percent = 0.0f;      // Smoothed loop load / حمل الحلقة بعد التنعيم / 平滑后的环路负载

// Thread-safe settings copy
// نسخ الإعدادات بأمان مؤشرات
// 中文: 线程安全设置复制
void CopySettingsLocked(FlightSettings* destination) {
  portENTER_CRITICAL(&g_state_lock);
  *destination = g_settings;
  portEXIT_CRITICAL(&g_state_lock);
}

// Thread-safe telemetry copy
// نسخ التليمترية بأمان مؤشرات
// 中文: 线程安全遥测复制
void CopyTelemetryLocked(TelemetrySnapshot* destination) {
  portENTER_CRITICAL(&g_state_lock);
  *destination = g_telemetry;
  portEXIT_CRITICAL(&g_state_lock);
}

// Thread-safe telemetry storage
// تخزين التليمترية بأمان مؤشرات
// 中文: 线程安全遥测存储
void StoreTelemetryLocked(const TelemetrySnapshot& telemetry) {
  portENTER_CRITICAL(&g_state_lock);
  g_telemetry = telemetry;
  portEXIT_CRITICAL(&g_state_lock);
}

// Thread-safe settings storage
// تخزين الإعدادات بأمان مؤشرات
// 中文: 线程安全设置存储
void StoreSettingsLocked(const FlightSettings& settings) {
  portENTER_CRITICAL(&g_state_lock);
  g_settings = settings;
  portEXIT_CRITICAL(&g_state_lock);
}

bool WifiSettingsChanged(const FlightSettings& current, const FlightSettings& next) {
  return strcmp(current.ap_ssid, next.ap_ssid) != 0 ||
         strcmp(current.ap_password, next.ap_password) != 0 ||
         current.sta_enabled != next.sta_enabled ||
         strcmp(current.sta_ssid, next.sta_ssid) != 0 ||
         strcmp(current.sta_password, next.sta_password) != 0;
}

void SetArmingReason(TelemetrySnapshot* telemetry, const char* reason) {
  if (telemetry == nullptr || reason == nullptr) return;
  strncpy(telemetry->arming_reason, reason, sizeof(telemetry->arming_reason) - 1);
  telemetry->arming_reason[sizeof(telemetry->arming_reason) - 1] = '\0';
}

void ResetControllers() {
  g_roll_pid.Reset();
  g_pitch_pid.Reset();
  g_yaw_pid.Reset();
}

bool ImuCalibratedForFlight(const ImuSnapshot& imu) {
  return imu.ready && imu.calibrated;
}

void UpdateLoopRate(TelemetrySnapshot* telemetry) {
  ++g_loop_counter;
  const uint32_t now_ms = millis();
  if ((now_ms - g_last_loop_rate_ms) >= 1000) {
    telemetry->loop_hz = g_loop_counter;
    g_loop_counter = 0;
    g_last_loop_rate_ms = now_ms;
  } else {
    telemetry->loop_hz = g_telemetry.loop_hz;
  }
}

void UpdateLoopLoad(TelemetrySnapshot* telemetry, uint32_t cycle_elapsed_us) {
  if (telemetry == nullptr) return;
  const float raw_load =
      fminf(100.0f, (static_cast<float>(cycle_elapsed_us) * 100.0f) /
                          static_cast<float>(DroneConfig::kControlPeriodUs));
  g_loop_load_percent = g_loop_load_percent * 0.82f + raw_load * 0.18f;
  telemetry->cpu_load_percent = g_loop_load_percent;
}

void ServiceBackgroundTasks() {
  const uint32_t now_ms = millis();
  if ((now_ms - g_last_web_service_ms) >= DroneConfig::kWebServicePeriodMs) {
    g_last_web_service_ms = now_ms;
    WebUi::loop();
  }

  const uint32_t now_us = micros();
  if ((now_us - g_last_gps_refresh_us) >= DroneConfig::kGpsRefreshPeriodUs) {
    g_last_gps_refresh_us = now_us;
    GpsManager::update();
  }

  TelemetrySnapshot telemetry{};
  CopyTelemetryLocked(&telemetry);
  TftStatus::update(telemetry);
}

void FillWifiInfo(TelemetrySnapshot* telemetry, const FlightSettings& settings) {
  strncpy(telemetry->ap_ssid, settings.ap_ssid, sizeof(telemetry->ap_ssid) - 1);
  telemetry->ap_ssid[sizeof(telemetry->ap_ssid) - 1] = '\0';
  strncpy(telemetry->ap_password, settings.ap_password, sizeof(telemetry->ap_password) - 1);
  telemetry->ap_password[sizeof(telemetry->ap_password) - 1] = '\0';
  telemetry->sta_enabled = settings.sta_enabled;
  telemetry->sta_connected = WifiManager::stationConnected();
  telemetry->sta_rssi = WifiManager::stationRssi();
  strncpy(telemetry->sta_ssid, settings.sta_ssid, sizeof(telemetry->sta_ssid) - 1);
  telemetry->sta_ssid[sizeof(telemetry->sta_ssid) - 1] = '\0';

  const String ap_ip_string = WifiManager::accessPointIpAddress();
  const String sta_ip_string = WifiManager::stationIpAddress();
  const String ip_string = WebUi::ipAddress();
  strncpy(telemetry->ap_ip_address, ap_ip_string.c_str(), sizeof(telemetry->ap_ip_address) - 1);
  telemetry->ap_ip_address[sizeof(telemetry->ap_ip_address) - 1] = '\0';
  strncpy(telemetry->sta_ip_address, sta_ip_string.c_str(), sizeof(telemetry->sta_ip_address) - 1);
  telemetry->sta_ip_address[sizeof(telemetry->sta_ip_address) - 1] = '\0';
  strncpy(telemetry->ip_address, ip_string.c_str(), sizeof(telemetry->ip_address) - 1);
  telemetry->ip_address[sizeof(telemetry->ip_address) - 1] = '\0';
}

uint16_t SnapRcEndpoint(uint16_t value_us) {
  if (value_us <= (DroneConfig::kRcMinUs + DroneConfig::kRcEndpointSnapUs)) {
    return DroneConfig::kRcMinUs;
  }
  if (value_us >= (DroneConfig::kRcMaxUs - DroneConfig::kRcEndpointSnapUs)) {
    return DroneConfig::kRcMaxUs;
  }
  return value_us;
}

float ApplyAxisDirection(float normalized_value, int direction) {
  return direction >= 0 ? normalized_value : -normalized_value;
}

}  // namespace

void FlightController::setup() {
  SerialConsole::begin();

  g_settings = SettingsStore::Load();
  SerialConsole::printBootBanner(g_settings);
  g_roll_pid.Configure(g_settings.roll_pid);
  g_pitch_pid.Configure(g_settings.pitch_pid);
  g_yaw_pid.Configure(g_settings.yaw_pid);

  RcInput::begin();
  MotorOutput::begin();
  ImuManager::begin(0.0f, 0.0f);
  ImuManager::getLevelOffsets(&g_settings.level_roll_offset_deg, &g_settings.level_pitch_offset_deg);
  GpsManager::begin();
  WebUi::begin(g_settings);
  TftStatus::begin();
  MotorMixer::reset();
  SerialConsole::reportDrivers(MotorOutput::ready(), ImuManager::isReady(),
                               GpsManager::isEnabled());

  g_last_control_us = micros();
  g_last_loop_rate_ms = millis();
  g_last_web_service_ms = millis();
  g_last_gps_refresh_us = micros();
  memset(&g_telemetry, 0, sizeof(g_telemetry));
  g_telemetry.web_link_mode = DroneConfig::kWebLinkModeNormal;
  g_telemetry.web_rx_allowed = true;
  g_telemetry.web_poll_hint_ms = DroneConfig::kWebPollNormalMs;
  g_telemetry.web_heavy_hint_ms = DroneConfig::kWebHeavyUpdateNormalMs;
  g_telemetry.web_link_ch8_us = DroneConfig::kRcMidUs;
}

void FlightController::loop() {
  ImuManager::update();

  const uint32_t now_us = micros();
  if ((now_us - g_last_control_us) < DroneConfig::kControlPeriodUs) {
    ServiceBackgroundTasks();
    return;
  }

  const float dt_seconds = static_cast<float>(now_us - g_last_control_us) / 1000000.0f;
  g_last_control_us = now_us;

  FlightSettings settings;
  CopySettingsLocked(&settings);

  RcSnapshot rc = RcInput::snapshot();
  ImuSnapshot imu = ImuManager::snapshot();
  GpsSnapshot gps = GpsManager::snapshot();
  const uint8_t web_link_mode = WebLink::DecodeMode(rc);
  const bool web_rx_allowed = WebLink::RxAllowed(web_link_mode);
  const uint16_t roll_channel_us = SnapRcEndpoint(rc.channels[DroneConfig::kRcRollIndex]);
  const uint16_t pitch_channel_us = SnapRcEndpoint(rc.channels[DroneConfig::kRcPitchIndex]);
  const uint16_t yaw_channel_us = SnapRcEndpoint(rc.channels[DroneConfig::kRcYawIndex]);
  const uint16_t throttle_channel_us =
      SnapRcEndpoint(rc.channels[DroneConfig::kRcThrottleIndex]);

  const float deadband = static_cast<float>(DroneConfig::kRcDeadbandUs) / 500.0f;
  float roll_input =
      ControllerMath::ApplyExpo(ControllerMath::ApplyDeadband(
                                    ApplyAxisDirection(
                                        ControllerMath::NormalizeStick(roll_channel_us,
                                                                       DroneConfig::kRcMidUs),
                                        DroneConfig::kRcRollDirection),
                                    deadband),
                                settings.rc_expo);
  float pitch_input =
      ControllerMath::ApplyExpo(ControllerMath::ApplyDeadband(
                                    ApplyAxisDirection(
                                        ControllerMath::NormalizeStick(pitch_channel_us,
                                                                       DroneConfig::kRcMidUs),
                                        DroneConfig::kRcPitchDirection),
                                    deadband),
                                settings.rc_expo);
  float yaw_input =
      ControllerMath::ApplyExpo(ControllerMath::ApplyDeadband(
                                    ApplyAxisDirection(
                                        ControllerMath::NormalizeStick(yaw_channel_us,
                                                                       DroneConfig::kRcMidUs),
                                        DroneConfig::kRcYawDirection),
                                    deadband),
                                settings.rc_expo);
  const float throttle_norm = ControllerMath::NormalizeThrottle(
      throttle_channel_us, DroneConfig::kRcMinUs, DroneConfig::kRcMaxUs);

    const bool arm_switch = rc.fresh[DroneConfig::kRcArmIndex] &&
                rc.channels[DroneConfig::kRcArmIndex] >=
                  DroneConfig::kRcSwitchOnUs;
    const bool sensor_switch = rc.fresh[DroneConfig::kRcSensorIndex] &&
                 rc.channels[DroneConfig::kRcSensorIndex] >=
                   DroneConfig::kRcSwitchOnUs;
  const bool imu_calibrated = ImuCalibratedForFlight(imu);
    const bool stabilize_requested = sensor_switch && imu_calibrated;

  if (!rc.frame_fresh || !arm_switch || !imu_calibrated) {
    g_armed = false;
  } else if (!g_armed && throttle_norm <= 0.05f) {
    g_armed = true;
  }

  TelemetrySnapshot telemetry{};
  telemetry.uptime_ms = millis();
  telemetry.armed = g_armed;
  telemetry.stabilize_mode = stabilize_requested;
  telemetry.rc_ok = rc.frame_fresh;
  telemetry.rc_failsafe = !rc.frame_fresh;
  telemetry.imu_ok = imu.ready;
  telemetry.imu_calibrated = imu.calibrated;
  telemetry.gps_ok = gps.healthy;
  telemetry.gps_fix = gps.fix;
  telemetry.gps_home_set = gps.home_set;
  telemetry.web_link_mode = web_link_mode;
  telemetry.web_rx_allowed = web_rx_allowed;
  telemetry.web_poll_hint_ms = WebLink::PollHintMs(web_link_mode);
  telemetry.web_heavy_hint_ms = WebLink::HeavyUpdateHintMs(web_link_mode);
  telemetry.web_link_ch8_us = rc.channels[DroneConfig::kRcAux2Index];
  telemetry.web_link_ch8_fresh = rc.fresh[DroneConfig::kRcAux2Index];
  telemetry.arm_switch_active = arm_switch;
  telemetry.sensor_switch_active = sensor_switch;
  telemetry.motor_output_ok = MotorOutput::ready();
  telemetry.throttle_percent = throttle_norm * 100.0f;
  telemetry.battery_available = false;
  telemetry.battery_voltage = 0.0f;
  telemetry.cpu_load_percent = g_loop_load_percent;
  telemetry.roll_deg = imu.roll_deg;
  telemetry.pitch_deg = imu.pitch_deg;
  telemetry.yaw_deg = imu.yaw_deg;
  telemetry.imu_cal_sys = imu.cal_sys;
  telemetry.imu_cal_gyro = imu.cal_gyro;
  telemetry.imu_cal_accel = imu.cal_accel;
  telemetry.imu_cal_mag = imu.cal_mag;
  telemetry.gps_satellites = gps.satellites;
  telemetry.gps_hdop = gps.hdop;
  telemetry.gps_speed_mps = gps.speed_mps;
  telemetry.gps_altitude_m = gps.altitude_m;
  telemetry.gps_course_deg = gps.course_deg;
  telemetry.gps_distance_home_m = gps.distance_home_m;
  telemetry.gps_bearing_home_deg = gps.bearing_home_deg;
  telemetry.gps_latitude = gps.latitude;
  telemetry.gps_longitude = gps.longitude;
  telemetry.gps_home_latitude = gps.home_latitude;
  telemetry.gps_home_longitude = gps.home_longitude;
  UpdateLoopRate(&telemetry);

  for (uint8_t i = 0; i < 8; ++i) {
    telemetry.rc_channels[i] = rc.channels[i];
    telemetry.rc_fresh[i] = rc.fresh[i];
  }

  if (!rc.fresh[DroneConfig::kRcArmIndex]) {
    SetArmingReason(&telemetry, "ARM channel lost");
  } else if (!rc.frame_fresh) {
    SetArmingReason(&telemetry, "RC signal lost");
  } else if (!imu.ready) {
    SetArmingReason(&telemetry, "IMU not detected");
  } else if (!imu_calibrated) {
    SetArmingReason(&telemetry, "IMU calibration in progress");
  } else if (!arm_switch) {
    SetArmingReason(&telemetry, "Arm switch is OFF");
  } else if (!g_armed) {
    SetArmingReason(&telemetry, "Lower throttle to arm");
  } else {
    SetArmingReason(&telemetry, "ARMED");
  }

  if (!g_armed) {
    MotorOutput::stopAll();
    ResetControllers();
    MotorMixer::reset();
    for (uint8_t i = 0; i < 4; ++i) {
      telemetry.motor_us[i] = DroneConfig::kEscMinUs;
    }
    strncpy(telemetry.mode_name, stabilize_requested ? "STAB" : "MANUAL",
            sizeof(telemetry.mode_name) - 1);
    FillWifiInfo(&telemetry, settings);
    UpdateLoopLoad(&telemetry, micros() - now_us);
    StoreTelemetryLocked(telemetry);
    return;
  }

  const uint16_t base_throttle_us = static_cast<uint16_t>(
      lroundf(settings.motor_idle_us +
              throttle_norm *
                  static_cast<float>(DroneConfig::kEscMaxUs - settings.motor_idle_us)));

  float roll_setpoint_deg = 0.0f;
  float pitch_setpoint_deg = 0.0f;
  float yaw_setpoint_dps = yaw_input * DroneConfig::kYawRateMaxDps;
  float roll_mix_us = 0.0f;
  float pitch_mix_us = 0.0f;
  float yaw_mix_us = 0.0f;

  if (stabilize_requested) {
    roll_setpoint_deg = roll_input * settings.max_angle_deg;
    pitch_setpoint_deg = pitch_input * settings.max_angle_deg;
    const float roll_feedback_deg = imu.roll_deg;
    const float pitch_feedback_deg = imu.pitch_deg;
    const float roll_feedback_rate_dps = imu.roll_rate_dps;
    const float pitch_feedback_rate_dps = imu.pitch_rate_dps;
    const float yaw_feedback_rate_dps = imu.yaw_rate_dps;

    if (throttle_norm <= 0.08f) {
      ResetControllers();
    }

    roll_mix_us =
        g_roll_pid.Update(roll_setpoint_deg - roll_feedback_deg, roll_feedback_rate_dps,
                          dt_seconds);
    pitch_mix_us =
        g_pitch_pid.Update(pitch_setpoint_deg - pitch_feedback_deg, pitch_feedback_rate_dps,
                           dt_seconds);
    yaw_mix_us =
        g_yaw_pid.Update(yaw_setpoint_dps - yaw_feedback_rate_dps, yaw_feedback_rate_dps,
                         dt_seconds);
  } else {
    ResetControllers();
    roll_mix_us = roll_input * settings.manual_mix_us;
    pitch_mix_us = pitch_input * settings.manual_mix_us;
    yaw_mix_us = yaw_input * settings.yaw_mix_us;
  }

  uint16_t motor_us[4] = {DroneConfig::kEscMinUs, DroneConfig::kEscMinUs,
                          DroneConfig::kEscMinUs, DroneConfig::kEscMinUs};
  MotorMixer::mix(base_throttle_us, pitch_mix_us, roll_mix_us, yaw_mix_us, settings, motor_us);
  MotorOutput::write(motor_us);

  for (uint8_t i = 0; i < 4; ++i) {
    telemetry.motor_us[i] = motor_us[i];
  }
  telemetry.roll_setpoint_deg = roll_setpoint_deg;
  telemetry.pitch_setpoint_deg = pitch_setpoint_deg;
  telemetry.yaw_setpoint_dps = yaw_setpoint_dps;
  telemetry.roll_pid_output = roll_mix_us;
  telemetry.pitch_pid_output = pitch_mix_us;
  telemetry.yaw_pid_output = yaw_mix_us;
  strncpy(telemetry.mode_name, stabilize_requested ? "STAB" : "MANUAL",
          sizeof(telemetry.mode_name) - 1);

  FillWifiInfo(&telemetry, settings);
  UpdateLoopLoad(&telemetry, micros() - now_us);
  StoreTelemetryLocked(telemetry);
}

TelemetrySnapshot FlightController::telemetry() {
  TelemetrySnapshot telemetry{};
  CopyTelemetryLocked(&telemetry);
  return telemetry;
}

FlightSettings FlightController::settings() {
  FlightSettings settings{};
  CopySettingsLocked(&settings);
  return settings;
}

bool FlightController::updateSettings(const FlightSettings& requested, String* message) {
  if (g_armed) {
    if (message != nullptr) *message = "Disarm before saving settings";
    return false;
  }

  const FlightSettings current = settings();
  FlightSettings next = requested;
  next.version = DroneConfig::kSettingsVersion;
  SettingsStore::Sanitize(&next);

  if (!SettingsStore::Save(next)) {
    if (message != nullptr) *message = "Failed to save settings";
    return false;
  }

  StoreSettingsLocked(next);
  g_roll_pid.Configure(next.roll_pid);
  g_pitch_pid.Configure(next.pitch_pid);
  g_yaw_pid.Configure(next.yaw_pid);
  ImuManager::setLevelOffsets(next.level_roll_offset_deg, next.level_pitch_offset_deg);
  if (WifiSettingsChanged(current, next)) {
    WebUi::scheduleReconfigureNetwork(next);
  }

  if (message != nullptr) *message = "Settings saved";
  return true;
}

bool FlightController::calibrateLevel(String* message) {
  if (g_armed) {
    if (message != nullptr) *message = "Disarm before calibration";
    return false;
  }

  const ImuSnapshot imu = ImuManager::snapshot();
  if (!imu.ready) {
    if (message != nullptr) *message = "IMU is not ready";
    return false;
  }
  if (!imu.calibrated) {
    if (message != nullptr) *message = "Wait until IMU calibration is ready before level trim";
    return false;
  }

  float roll_offset = 0.0f;
  float pitch_offset = 0.0f;
  if (!ImuManager::captureLevelOffsets(&roll_offset, &pitch_offset)) {
    if (message != nullptr) *message = "Failed to capture stable IMU samples";
    return false;
  }

  FlightSettings next = settings();
  next.level_roll_offset_deg = roll_offset;
  next.level_pitch_offset_deg = pitch_offset;
  return updateSettings(next, message);
}

bool FlightController::optimizeWeights(String* message) {
  if (g_armed) {
    if (message != nullptr) *message = "Disarm before optimizing balance";
    return false;
  }

  const ImuSnapshot imu = ImuManager::snapshot();
  if (!imu.ready) {
    if (message != nullptr) *message = "IMU is not ready";
    return false;
  }

  FlightSettings next = settings();
  const float roll_error = imu.roll_deg;
  const float pitch_error = imu.pitch_deg;
  const float delta[4] = {
      (-pitch_error + roll_error) * DroneConfig::kWeightOptimizeGain,
      (pitch_error + roll_error) * DroneConfig::kWeightOptimizeGain,
      (pitch_error - roll_error) * DroneConfig::kWeightOptimizeGain,
      (-pitch_error - roll_error) * DroneConfig::kWeightOptimizeGain,
  };

  for (uint8_t i = 0; i < 4; ++i) {
    next.motor_weight[i] += delta[i];
  }

  return updateSettings(next, message);
}

bool FlightController::setGpsHome(String* message) {
  if (!GpsManager::setHomeToCurrent()) {
    if (message != nullptr) *message = "GPS fix is not ready";
    return false;
  }
  if (message != nullptr) *message = "Home updated";
  return true;
}

void FlightController::clearGpsHome() {
  GpsManager::clearHome();
}

void FlightController::disarm() {
  g_armed = false;
  ResetControllers();
  MotorMixer::reset();
  MotorOutput::stopAll();
}
