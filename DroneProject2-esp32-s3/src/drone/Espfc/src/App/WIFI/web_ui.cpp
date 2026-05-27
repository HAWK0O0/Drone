#include "WIFI/web_ui.h"

#include <ArduinoJson.h>
#include <WebServer.h>
#include <stdlib.h>
#include <string.h>

#include "Connect/web_link.h"
#include "Control/flight_controller.h"
#include "INPUT/drone_config.h"
#include "WIFI/web_assets.h"
#include "WIFI/wifi_manager.h"

namespace {

WebServer g_server(80);
bool g_server_started = false;
bool g_network_reconfigure_pending = false;
uint32_t g_network_reconfigure_after_ms = 0;
FlightSettings g_network_reconfigure_settings{};

constexpr uint32_t kDeferredNetworkReconfigureMs = 600;
constexpr size_t kStateFastJsonCapacity = 3072;
constexpr size_t kStateHeavyJsonCapacity = 2560;

void AppendPinArray(const uint8_t* pins, size_t count, JsonArray array) {
  for (size_t i = 0; i < count; ++i) {
    array.add(pins[i]);
  }
}

void SendJson(int code, JsonDocument* doc) {
  String response;
  serializeJson(*doc, response);
  g_server.sendHeader("Cache-Control", "no-store");
  g_server.sendHeader("Connection", "close");
  g_server.send(code, "application/json", response);
}

void SendResult(bool ok, const String& message, int code) {
  DynamicJsonDocument doc(256);
  doc["ok"] = ok;
  doc["message"] = message;
  SendJson(code, &doc);
}

bool IsWritableTopLevelField(const char* key) {
  static const char* kWritableFields[] = {
      "rollPid",          "pitchPid",       "yawPid",           "maxAngleDeg",      "rcExpo",
      "manualMixUs",      "yawMixUs",       "motorIdleUs",      "motorWeight",
      "motorTrimUs",      "levelRollOffsetDeg", "levelPitchOffsetDeg", "wifi",
  };

  for (size_t i = 0; i < (sizeof(kWritableFields) / sizeof(kWritableFields[0])); ++i) {
    if (strcmp(key, kWritableFields[i]) == 0) return true;
  }
  return false;
}

bool IsReadOnlyTopLevelField(const char* key) {
  static const char* kReadOnlyFields[] = {
      "version", "board", "resources", "safety", "features", "restrictions",
  };

  for (size_t i = 0; i < (sizeof(kReadOnlyFields) / sizeof(kReadOnlyFields[0])); ++i) {
    if (strcmp(key, kReadOnlyFields[i]) == 0) return true;
  }
  return false;
}

bool TryReadNumberAsFloat(JsonVariantConst value, float* out) {
  if (value.is<float>() || value.is<double>() || value.is<int>() || value.is<long>() ||
      value.is<unsigned int>() || value.is<unsigned long>()) {
    *out = value.as<float>();
    return true;
  }
  if (value.is<const char*>() || value.is<String>()) {
    String text = value.as<String>();
    text.trim();
    text.replace(',', '.');
    if (text.length() == 0) return false;
    char* end_ptr = nullptr;
    const float parsed = strtof(text.c_str(), &end_ptr);
    if (end_ptr == text.c_str()) return false;
    while (*end_ptr == ' ' || *end_ptr == '\t') ++end_ptr;
    if (*end_ptr != '\0') return false;
    *out = parsed;
    return true;
  }
  return false;
}

bool TryReadNumberAsInt32(JsonVariantConst value, int32_t* out) {
  if (value.is<int>() || value.is<long>() || value.is<unsigned int>() || value.is<unsigned long>() ||
      value.is<float>() || value.is<double>()) {
    *out = value.as<int32_t>();
    return true;
  }
  if (value.is<const char*>() || value.is<String>()) {
    String text = value.as<String>();
    text.trim();
    if (text.length() == 0) return false;
    char* end_ptr = nullptr;
    const long parsed = strtol(text.c_str(), &end_ptr, 10);
    if (end_ptr == text.c_str()) return false;
    while (*end_ptr == ' ' || *end_ptr == '\t') ++end_ptr;
    if (*end_ptr != '\0') return false;
    *out = static_cast<int32_t>(parsed);
    return true;
  }
  return false;
}

String ScopedFieldName(const char* scope, const char* field) {
  if (scope == nullptr || scope[0] == '\0') return String(field);
  return String(scope) + "." + field;
}

void AddWarning(JsonArray warnings, const String& text) {
  warnings.add(text);
}

void AddIgnored(JsonArray ignored, JsonArray warnings, const String& field, const String& reason) {
  ignored.add(field);
  AddWarning(warnings, field + " " + reason);
}

void AddApplied(JsonArray applied, const String& field) {
  applied.add(field);
}

void WarnUnknownNestedFields(JsonObjectConst object, const char* scope, const char* const* known,
                             size_t known_count, JsonArray ignored, JsonArray warnings) {
  for (JsonPairConst kv : object) {
    const char* key = kv.key().c_str();
    bool known_key = false;
    for (size_t i = 0; i < known_count; ++i) {
      if (strcmp(key, known[i]) == 0) {
        known_key = true;
        break;
      }
    }
    if (!known_key) {
      AddIgnored(ignored, warnings, ScopedFieldName(scope, key), "is unknown and was ignored");
    }
  }
}

void ApplyFloatField(JsonObjectConst object, const char* key, float* target, const char* scope,
                     JsonArray applied, JsonArray ignored, JsonArray warnings) {
  if (!object.containsKey(key)) return;
  const String field_name = ScopedFieldName(scope, key);
  float parsed = 0.0f;
  if (!TryReadNumberAsFloat(object[key], &parsed)) {
    AddIgnored(ignored, warnings, field_name, "expects a numeric value");
    return;
  }
  *target = parsed;
  AddApplied(applied, field_name);
}

void ApplyUInt16Field(JsonObjectConst object, const char* key, uint16_t* target, const char* scope,
                      JsonArray applied, JsonArray ignored, JsonArray warnings) {
  if (!object.containsKey(key)) return;
  const String field_name = ScopedFieldName(scope, key);
  int32_t parsed = 0;
  if (!TryReadNumberAsInt32(object[key], &parsed)) {
    AddIgnored(ignored, warnings, field_name, "expects a numeric value");
    return;
  }
  if (parsed < 0) parsed = 0;
  *target = static_cast<uint16_t>(parsed);
  AddApplied(applied, field_name);
}

void ApplyStringField(JsonObjectConst object, const char* key, char* target, size_t target_len,
                      const char* scope, JsonArray applied, JsonArray ignored, JsonArray warnings) {
  if (!object.containsKey(key)) return;
  const String field_name = ScopedFieldName(scope, key);

  JsonVariantConst value = object[key];
  if (!(value.is<const char*>() || value.is<String>())) {
    AddIgnored(ignored, warnings, field_name, "expects a string");
    return;
  }

  String text_value = value.as<String>();
  if (target_len == 0) {
    AddIgnored(ignored, warnings, field_name, "has an invalid target buffer");
    return;
  }

  strncpy(target, text_value.c_str(), target_len - 1);
  target[target_len - 1] = '\0';
  AddApplied(applied, field_name);
}

void ApplyFloatArrayField(JsonObjectConst object, const char* key, float* target, size_t count,
                          JsonArray applied, JsonArray ignored, JsonArray warnings) {
  if (!object.containsKey(key)) return;

  if (!object[key].is<JsonArray>()) {
    AddIgnored(ignored, warnings, key, "expects an array");
    return;
  }

  JsonArrayConst values = object[key].as<JsonArrayConst>();
  for (size_t i = 0; i < values.size(); ++i) {
    const String field_name = String(key) + "[" + String(static_cast<unsigned int>(i)) + "]";
    if (i >= count) {
      AddIgnored(ignored, warnings, field_name, "is out of range and was ignored");
      continue;
    }

    JsonVariantConst item = values[i];
    if (item.isNull()) continue;

    float parsed = 0.0f;
    if (!TryReadNumberAsFloat(item, &parsed)) {
      AddIgnored(ignored, warnings, field_name, "expects a numeric value");
      continue;
    }
    target[i] = parsed;
    AddApplied(applied, field_name);
  }
}

void ApplyInt16ArrayField(JsonObjectConst object, const char* key, int16_t* target, size_t count,
                          JsonArray applied, JsonArray ignored, JsonArray warnings) {
  if (!object.containsKey(key)) return;

  if (!object[key].is<JsonArray>()) {
    AddIgnored(ignored, warnings, key, "expects an array");
    return;
  }

  JsonArrayConst values = object[key].as<JsonArrayConst>();
  for (size_t i = 0; i < values.size(); ++i) {
    const String field_name = String(key) + "[" + String(static_cast<unsigned int>(i)) + "]";
    if (i >= count) {
      AddIgnored(ignored, warnings, field_name, "is out of range and was ignored");
      continue;
    }

    JsonVariantConst item = values[i];
    if (item.isNull()) continue;

    int32_t parsed = 0;
    if (!TryReadNumberAsInt32(item, &parsed)) {
      AddIgnored(ignored, warnings, field_name, "expects a numeric value");
      continue;
    }
    target[i] = static_cast<int16_t>(parsed);
    AddApplied(applied, field_name);
  }
}

void SerializeSettingsJson(const FlightSettings& settings, bool readonly_web, JsonDocument* doc) {
  JsonObject roll_pid = doc->createNestedObject("rollPid");
  roll_pid["p"] = settings.roll_pid.kp;
  roll_pid["i"] = settings.roll_pid.ki;
  roll_pid["d"] = settings.roll_pid.kd;
  roll_pid["integralLimit"] = settings.roll_pid.integral_limit;
  roll_pid["outputLimit"] = settings.roll_pid.output_limit;

  JsonObject pitch_pid = doc->createNestedObject("pitchPid");
  pitch_pid["p"] = settings.pitch_pid.kp;
  pitch_pid["i"] = settings.pitch_pid.ki;
  pitch_pid["d"] = settings.pitch_pid.kd;
  pitch_pid["integralLimit"] = settings.pitch_pid.integral_limit;
  pitch_pid["outputLimit"] = settings.pitch_pid.output_limit;

  JsonObject yaw_pid = doc->createNestedObject("yawPid");
  yaw_pid["p"] = settings.yaw_pid.kp;
  yaw_pid["i"] = settings.yaw_pid.ki;
  yaw_pid["d"] = settings.yaw_pid.kd;
  yaw_pid["integralLimit"] = settings.yaw_pid.integral_limit;
  yaw_pid["outputLimit"] = settings.yaw_pid.output_limit;

  (*doc)["version"] = settings.version;
  (*doc)["maxAngleDeg"] = settings.max_angle_deg;
  (*doc)["rcExpo"] = settings.rc_expo;
  (*doc)["manualMixUs"] = settings.manual_mix_us;
  (*doc)["yawMixUs"] = settings.yaw_mix_us;
  (*doc)["motorIdleUs"] = settings.motor_idle_us;
  (*doc)["levelRollOffsetDeg"] = settings.level_roll_offset_deg;
  (*doc)["levelPitchOffsetDeg"] = settings.level_pitch_offset_deg;

  JsonArray weights = doc->createNestedArray("motorWeight");
  JsonArray trims = doc->createNestedArray("motorTrimUs");
  for (uint8_t i = 0; i < 4; ++i) {
    weights.add(settings.motor_weight[i]);
    trims.add(settings.motor_trim_us[i]);
  }

  JsonObject wifi = doc->createNestedObject("wifi");
  wifi["apSsid"] = settings.ap_ssid;
  wifi["apPassword"] = settings.ap_password;
  wifi["staEnabled"] = settings.sta_enabled;
  wifi["staSsid"] = settings.sta_ssid;
  wifi["staPassword"] = settings.sta_password;

  JsonObject board = doc->createNestedObject("board");
  board["name"] = DroneConfig::kFirmwareName;
  board["controlHz"] = 1000000UL / DroneConfig::kControlPeriodUs;
  board["imuHz"] = 1000000UL / DroneConfig::kImuReadPeriodUs;
  board["gpsHz"] = 1000000UL / DroneConfig::kGpsRefreshPeriodUs;
  board["motorPwmHz"] = DroneConfig::kMotorPwmHz;
  board["escMinUs"] = DroneConfig::kEscMinUs;
  board["escMaxUs"] = DroneConfig::kEscMaxUs;
  board["readonlyWeb"] = readonly_web;

  JsonObject resources = doc->createNestedObject("resources");
  AppendPinArray(DroneConfig::kMotorPins, 4, resources.createNestedArray("motorPins"));

  JsonObject receiver = resources.createNestedObject("receiver");
  receiver["protocol"] = DroneConfig::kRcProtocol;
  receiver["rxPin"] = DroneConfig::kRcSbusRxPin;
  receiver["baud"] = DroneConfig::kRcSbusBaud;
  receiver["inverted"] = DroneConfig::kRcSbusInvert;

  JsonObject gps_ports = resources.createNestedObject("gps");
  gps_ports["rx"] = DroneConfig::kGpsRxPin;
  gps_ports["tx"] = DroneConfig::kGpsTxPin;
  gps_ports["baud"] = DroneConfig::kGpsBaud;

  JsonObject i2c = resources.createNestedObject("i2c");
  i2c["scl"] = DroneConfig::kI2cSclPin;
  i2c["sda"] = DroneConfig::kI2cSdaPin;

  JsonObject tft = resources.createNestedObject("tft");
  tft["rst"] = DroneConfig::kTftRstPin;
  tft["dc"] = DroneConfig::kTftDcPin;
  tft["mosi"] = DroneConfig::kTftMosiPin;
  tft["sck"] = DroneConfig::kTftSckPin;
  tft["cs"] = DroneConfig::kTftCsPin;
  tft["backlight"] = DroneConfig::kTftBacklightPin;

  JsonObject safety = doc->createNestedObject("safety");
  safety["failsafeMs"] = DroneConfig::kRcFailsafeUs / 1000U;
  safety["armChannel"] = DroneConfig::kRcArmIndex + 1;
  safety["stabilizeChannel"] = DroneConfig::kRcSensorIndex + 1;
  safety["armThresholdUs"] = DroneConfig::kRcSwitchOnUs;
  safety["throttleArmPct"] = 5;

  JsonObject features = doc->createNestedObject("features");
  features["batterySense"] = false;
  features["ledStrip"] = false;
  features["blackbox"] = false;
  features["cliUsb"] = true;
  features["cameraFlight"] = false;
  features["sdCardFlight"] = false;

  JsonArray restrictions = doc->createNestedArray("restrictions");
  restrictions.add("Disconnect the camera during flight because GPIO4..18 are reused.");
  restrictions.add("Keep the SD slot empty because GPIO37..40 drive the ESC outputs.");
  restrictions.add("Feed board 5V from one ESC BEC only and isolate the other red wires.");
  restrictions.add("This build targets the no-PSRAM ESP32-S3 profile.");
}

bool RejectIfWebRxBlocked() {
  if (!DroneConfig::kEnableCh8WebLinkGate) return false;

  const TelemetrySnapshot telemetry = FlightController::telemetry();
  if (telemetry.web_rx_allowed) return false;

  DynamicJsonDocument doc(512);
  doc["ok"] = false;
  doc["message"] =
      "CH8 is in 1500 broadcast-only mode. Incoming commands are blocked.";
  doc["webLinkMode"] = WebLink::ModeName(telemetry.web_link_mode);
  doc["webRxAllowed"] = telemetry.web_rx_allowed;
  SendJson(403, &doc);
  return true;
}

void HandleRoot() {
  g_server.sendHeader("Cache-Control", "no-store");
  g_server.sendHeader("Connection", "close");
  g_server.send_P(200, "text/html", kWebDashboardHtml);
}

void FillStateFast(JsonDocument* doc, const TelemetrySnapshot& telemetry) {
  (*doc)["ok"] = true;
  (*doc)["armed"] = telemetry.armed;
  (*doc)["mode"] = telemetry.mode_name;
  (*doc)["loopHz"] = telemetry.loop_hz;
  (*doc)["uptimeMs"] = telemetry.uptime_ms;
  (*doc)["cpuLoadPct"] = telemetry.cpu_load_percent;
  (*doc)["batteryAvailable"] = telemetry.battery_available;
  (*doc)["batteryVoltage"] = telemetry.battery_voltage;
  (*doc)["rcOk"] = telemetry.rc_ok;
  (*doc)["rcFailsafe"] = telemetry.rc_failsafe;
  (*doc)["imuOk"] = telemetry.imu_ok;
  (*doc)["imuCalibrated"] = telemetry.imu_calibrated;
  (*doc)["gpsOk"] = telemetry.gps_ok;
  (*doc)["gpsFix"] = telemetry.gps_fix;
  (*doc)["gpsHomeSet"] = telemetry.gps_home_set;
  (*doc)["armSwitch"] = telemetry.arm_switch_active;
  (*doc)["sensorSwitch"] = telemetry.sensor_switch_active;
  (*doc)["motorOutputOk"] = telemetry.motor_output_ok;
  (*doc)["throttlePct"] = telemetry.throttle_percent;
  (*doc)["armingReason"] = telemetry.arming_reason;
  (*doc)["roll"] = telemetry.roll_deg;
  (*doc)["pitch"] = telemetry.pitch_deg;
  (*doc)["yaw"] = telemetry.yaw_deg;
  (*doc)["webLinkMode"] = WebLink::ModeName(telemetry.web_link_mode);
  (*doc)["webRxAllowed"] = telemetry.web_rx_allowed;
  (*doc)["webPollHintMs"] = telemetry.web_poll_hint_ms;
  (*doc)["webHeavyHintMs"] = telemetry.web_heavy_hint_ms;
  (*doc)["webLinkCh8Us"] = telemetry.web_link_ch8_us;
  (*doc)["webLinkCh8Fresh"] = telemetry.web_link_ch8_fresh;

  JsonArray rc = doc->createNestedArray("rc");
  JsonArray rc_fresh = doc->createNestedArray("rcFresh");
  JsonArray motors = doc->createNestedArray("motors");
  for (uint8_t i = 0; i < 8; ++i) {
    rc.add(telemetry.rc_channels[i]);
    rc_fresh.add(telemetry.rc_fresh[i]);
  }
  for (uint8_t i = 0; i < 4; ++i) {
    motors.add(telemetry.motor_us[i]);
  }

  JsonObject wifi = doc->createNestedObject("wifi");
  wifi["apIp"] = telemetry.ap_ip_address;
  wifi["staEnabled"] = telemetry.sta_enabled;
  wifi["staConnected"] = telemetry.sta_connected;
  wifi["staIp"] = telemetry.sta_ip_address;
  wifi["staRssi"] = telemetry.sta_rssi;
  wifi["ip"] = telemetry.ip_address;
}

void FillStateHeavy(JsonDocument* doc, const TelemetrySnapshot& telemetry) {
  (*doc)["ok"] = true;

  JsonObject imu_cal = doc->createNestedObject("imuCal");
  imu_cal["sys"] = telemetry.imu_cal_sys;
  imu_cal["gyro"] = telemetry.imu_cal_gyro;
  imu_cal["accel"] = telemetry.imu_cal_accel;
  imu_cal["mag"] = telemetry.imu_cal_mag;

  JsonObject gps = doc->createNestedObject("gps");
  gps["sat"] = telemetry.gps_satellites;
  gps["hdop"] = telemetry.gps_hdop;
  gps["speed"] = telemetry.gps_speed_mps;
  gps["alt"] = telemetry.gps_altitude_m;
  gps["course"] = telemetry.gps_course_deg;
  gps["distanceHome"] = telemetry.gps_distance_home_m;
  gps["bearingHome"] = telemetry.gps_bearing_home_deg;
  gps["lat"] = telemetry.gps_latitude;
  gps["lng"] = telemetry.gps_longitude;
  gps["homeLat"] = telemetry.gps_home_latitude;
  gps["homeLng"] = telemetry.gps_home_longitude;
  gps["homeSet"] = telemetry.gps_home_set;

  JsonObject wifi = doc->createNestedObject("wifi");
  wifi["apIp"] = telemetry.ap_ip_address;
  wifi["staEnabled"] = telemetry.sta_enabled;
  wifi["staConnected"] = telemetry.sta_connected;
  wifi["staIp"] = telemetry.sta_ip_address;
  wifi["staRssi"] = telemetry.sta_rssi;
  wifi["ip"] = telemetry.ip_address;
}

void HandleState() {
  const TelemetrySnapshot telemetry = FlightController::telemetry();

  DynamicJsonDocument doc(kStateFastJsonCapacity);
  FillStateFast(&doc, telemetry);
  SendJson(200, &doc);
}

void HandleStateHeavy() {
  const TelemetrySnapshot telemetry = FlightController::telemetry();

  DynamicJsonDocument doc(kStateHeavyJsonCapacity);
  FillStateHeavy(&doc, telemetry);
  SendJson(200, &doc);
}

void HandleSettingsGet() {
  const FlightSettings settings = FlightController::settings();
  const TelemetrySnapshot telemetry = FlightController::telemetry();

  DynamicJsonDocument doc(4096);
  doc["ok"] = true;
  SerializeSettingsJson(settings, !telemetry.web_rx_allowed, &doc);
  SendJson(200, &doc);
}

void HandleSettingsPost() {
  if (RejectIfWebRxBlocked()) return;

  DynamicJsonDocument response_doc(8192);
  response_doc["ok"] = true;
  response_doc["saved"] = false;

  JsonArray applied = response_doc.createNestedArray("applied");
  JsonArray ignored = response_doc.createNestedArray("ignored");
  JsonArray warnings = response_doc.createNestedArray("warnings");

  const String body = g_server.arg("plain");
  if (body.length() == 0) {
    response_doc["message"] = "Empty payload. Existing settings kept.";
    SendJson(200, &response_doc);
    return;
  }

  DynamicJsonDocument request_doc(4096);
  DeserializationError parse_error = deserializeJson(request_doc, body);
  if (parse_error) {
    response_doc["ok"] = false;
    response_doc["message"] = String("Invalid JSON: ") + parse_error.c_str();
    SendJson(400, &response_doc);
    return;
  }

  if (!request_doc.is<JsonObject>()) {
    response_doc["ok"] = false;
    response_doc["message"] = "JSON payload must be an object";
    SendJson(400, &response_doc);
    return;
  }

  FlightSettings merged = FlightController::settings();
  JsonObjectConst root = request_doc.as<JsonObjectConst>();

  for (JsonPairConst kv : root) {
    const char* key = kv.key().c_str();
    if (IsWritableTopLevelField(key)) continue;
    if (IsReadOnlyTopLevelField(key)) {
      AddIgnored(ignored, warnings, key, "is read-only and was ignored");
      continue;
    }
    AddIgnored(ignored, warnings, key, "is unknown and was ignored");
  }

  if (root.containsKey("rollPid")) {
    if (!root["rollPid"].is<JsonObject>()) {
      AddIgnored(ignored, warnings, "rollPid", "expects an object");
    } else {
      JsonObjectConst roll = root["rollPid"].as<JsonObjectConst>();
      ApplyFloatField(roll, "p", &merged.roll_pid.kp, "rollPid", applied, ignored, warnings);
      ApplyFloatField(roll, "i", &merged.roll_pid.ki, "rollPid", applied, ignored, warnings);
      ApplyFloatField(roll, "d", &merged.roll_pid.kd, "rollPid", applied, ignored, warnings);
      ApplyFloatField(roll, "integralLimit", &merged.roll_pid.integral_limit, "rollPid", applied,
                      ignored, warnings);
      ApplyFloatField(roll, "outputLimit", &merged.roll_pid.output_limit, "rollPid", applied,
                      ignored, warnings);
      static const char* kRollKnown[] = {"p", "i", "d", "integralLimit", "outputLimit"};
      WarnUnknownNestedFields(roll, "rollPid", kRollKnown,
                              sizeof(kRollKnown) / sizeof(kRollKnown[0]), ignored, warnings);
    }
  }

  if (root.containsKey("pitchPid")) {
    if (!root["pitchPid"].is<JsonObject>()) {
      AddIgnored(ignored, warnings, "pitchPid", "expects an object");
    } else {
      JsonObjectConst pitch = root["pitchPid"].as<JsonObjectConst>();
      ApplyFloatField(pitch, "p", &merged.pitch_pid.kp, "pitchPid", applied, ignored, warnings);
      ApplyFloatField(pitch, "i", &merged.pitch_pid.ki, "pitchPid", applied, ignored, warnings);
      ApplyFloatField(pitch, "d", &merged.pitch_pid.kd, "pitchPid", applied, ignored, warnings);
      ApplyFloatField(pitch, "integralLimit", &merged.pitch_pid.integral_limit, "pitchPid",
                      applied, ignored, warnings);
      ApplyFloatField(pitch, "outputLimit", &merged.pitch_pid.output_limit, "pitchPid", applied,
                      ignored, warnings);
      static const char* kPitchKnown[] = {"p", "i", "d", "integralLimit", "outputLimit"};
      WarnUnknownNestedFields(pitch, "pitchPid", kPitchKnown,
                              sizeof(kPitchKnown) / sizeof(kPitchKnown[0]), ignored, warnings);
    }
  }

  if (root.containsKey("yawPid")) {
    if (!root["yawPid"].is<JsonObject>()) {
      AddIgnored(ignored, warnings, "yawPid", "expects an object");
    } else {
      JsonObjectConst yaw = root["yawPid"].as<JsonObjectConst>();
      ApplyFloatField(yaw, "p", &merged.yaw_pid.kp, "yawPid", applied, ignored, warnings);
      ApplyFloatField(yaw, "i", &merged.yaw_pid.ki, "yawPid", applied, ignored, warnings);
      ApplyFloatField(yaw, "d", &merged.yaw_pid.kd, "yawPid", applied, ignored, warnings);
      ApplyFloatField(yaw, "integralLimit", &merged.yaw_pid.integral_limit, "yawPid", applied,
                      ignored, warnings);
      ApplyFloatField(yaw, "outputLimit", &merged.yaw_pid.output_limit, "yawPid", applied,
                      ignored, warnings);
      static const char* kYawKnown[] = {"p", "i", "d", "integralLimit", "outputLimit"};
      WarnUnknownNestedFields(yaw, "yawPid", kYawKnown,
                              sizeof(kYawKnown) / sizeof(kYawKnown[0]), ignored, warnings);
    }
  }

  ApplyFloatField(root, "maxAngleDeg", &merged.max_angle_deg, "", applied, ignored, warnings);
  ApplyFloatField(root, "rcExpo", &merged.rc_expo, "", applied, ignored, warnings);
  ApplyFloatField(root, "manualMixUs", &merged.manual_mix_us, "", applied, ignored, warnings);
  ApplyFloatField(root, "yawMixUs", &merged.yaw_mix_us, "", applied, ignored, warnings);
  ApplyUInt16Field(root, "motorIdleUs", &merged.motor_idle_us, "", applied, ignored, warnings);
  ApplyFloatField(root, "levelRollOffsetDeg", &merged.level_roll_offset_deg, "", applied, ignored,
                  warnings);
  ApplyFloatField(root, "levelPitchOffsetDeg", &merged.level_pitch_offset_deg, "", applied,
                  ignored, warnings);
  ApplyFloatArrayField(root, "motorWeight", merged.motor_weight, 4, applied, ignored, warnings);
  ApplyInt16ArrayField(root, "motorTrimUs", merged.motor_trim_us, 4, applied, ignored, warnings);

  if (root.containsKey("wifi")) {
    if (!root["wifi"].is<JsonObject>()) {
      AddIgnored(ignored, warnings, "wifi", "expects an object");
    } else {
      JsonObjectConst wifi = root["wifi"].as<JsonObjectConst>();
      ApplyStringField(wifi, "ssid", merged.ap_ssid, sizeof(merged.ap_ssid), "wifi", applied,
                       ignored, warnings);
      ApplyStringField(wifi, "password", merged.ap_password, sizeof(merged.ap_password), "wifi",
                       applied, ignored, warnings);
      ApplyStringField(wifi, "apSsid", merged.ap_ssid, sizeof(merged.ap_ssid), "wifi", applied,
                       ignored, warnings);
      ApplyStringField(wifi, "apPassword", merged.ap_password, sizeof(merged.ap_password), "wifi",
                       applied, ignored, warnings);
      ApplyStringField(wifi, "staSsid", merged.sta_ssid, sizeof(merged.sta_ssid), "wifi", applied,
                       ignored, warnings);
      ApplyStringField(wifi, "staPassword", merged.sta_password, sizeof(merged.sta_password),
                       "wifi", applied, ignored, warnings);
      if (wifi.containsKey("staEnabled")) {
        if (wifi["staEnabled"].is<bool>() || wifi["staEnabled"].is<int>()) {
          merged.sta_enabled = wifi["staEnabled"].as<bool>();
          AddApplied(applied, "wifi.staEnabled");
        } else {
          AddIgnored(ignored, warnings, "wifi.staEnabled", "expects a boolean value");
        }
      }
      static const char* kWifiKnown[] = {"ssid", "password", "apSsid", "apPassword", "staEnabled",
                                         "staSsid", "staPassword"};
      WarnUnknownNestedFields(wifi, "wifi", kWifiKnown,
                              sizeof(kWifiKnown) / sizeof(kWifiKnown[0]), ignored, warnings);
    }
  }

  if (applied.size() == 0) {
    response_doc["message"] = "No valid writable fields were provided";
    SendJson(200, &response_doc);
    return;
  }

  String message;
  const bool ok = FlightController::updateSettings(merged, &message);
  response_doc["ok"] = ok;
  response_doc["saved"] = ok;
  response_doc["message"] = message;
  SendJson(ok ? 200 : 400, &response_doc);
}

void HandlePidPost() {
  if (RejectIfWebRxBlocked()) return;

  DynamicJsonDocument response_doc(2048);
  response_doc["ok"] = true;
  response_doc["saved"] = false;
  JsonArray applied = response_doc.createNestedArray("applied");
  JsonArray ignored = response_doc.createNestedArray("ignored");
  JsonArray warnings = response_doc.createNestedArray("warnings");

  const String body = g_server.arg("plain");
  if (body.length() == 0) {
    response_doc["ok"] = false;
    response_doc["message"] = "Empty PID payload";
    SendJson(400, &response_doc);
    return;
  }

  DynamicJsonDocument request_doc(1024);
  DeserializationError parse_error = deserializeJson(request_doc, body);
  if (parse_error) {
    response_doc["ok"] = false;
    response_doc["message"] = String("Invalid JSON: ") + parse_error.c_str();
    SendJson(400, &response_doc);
    return;
  }

  if (!request_doc.is<JsonObject>()) {
    response_doc["ok"] = false;
    response_doc["message"] = "PID payload must be an object";
    SendJson(400, &response_doc);
    return;
  }

  FlightSettings merged = FlightController::settings();
  JsonObjectConst root = request_doc.as<JsonObjectConst>();

  // Flat PID fields are accepted for maximum client compatibility.
  ApplyFloatField(root, "rollP", &merged.roll_pid.kp, "", applied, ignored, warnings);
  ApplyFloatField(root, "rollI", &merged.roll_pid.ki, "", applied, ignored, warnings);
  ApplyFloatField(root, "rollD", &merged.roll_pid.kd, "", applied, ignored, warnings);
  ApplyFloatField(root, "pitchP", &merged.pitch_pid.kp, "", applied, ignored, warnings);
  ApplyFloatField(root, "pitchI", &merged.pitch_pid.ki, "", applied, ignored, warnings);
  ApplyFloatField(root, "pitchD", &merged.pitch_pid.kd, "", applied, ignored, warnings);
  ApplyFloatField(root, "yawP", &merged.yaw_pid.kp, "", applied, ignored, warnings);
  ApplyFloatField(root, "yawI", &merged.yaw_pid.ki, "", applied, ignored, warnings);
  ApplyFloatField(root, "yawD", &merged.yaw_pid.kd, "", applied, ignored, warnings);

  if (root.containsKey("rollPid")) {
    if (!root["rollPid"].is<JsonObject>()) {
      AddIgnored(ignored, warnings, "rollPid", "expects an object");
    } else {
      JsonObjectConst roll = root["rollPid"].as<JsonObjectConst>();
      ApplyFloatField(roll, "p", &merged.roll_pid.kp, "rollPid", applied, ignored, warnings);
      ApplyFloatField(roll, "kp", &merged.roll_pid.kp, "rollPid", applied, ignored, warnings);
      ApplyFloatField(roll, "i", &merged.roll_pid.ki, "rollPid", applied, ignored, warnings);
      ApplyFloatField(roll, "ki", &merged.roll_pid.ki, "rollPid", applied, ignored, warnings);
      ApplyFloatField(roll, "d", &merged.roll_pid.kd, "rollPid", applied, ignored, warnings);
      ApplyFloatField(roll, "kd", &merged.roll_pid.kd, "rollPid", applied, ignored, warnings);
      static const char* kRollKnown[] = {"p", "kp", "i", "ki", "d", "kd"};
      WarnUnknownNestedFields(roll, "rollPid", kRollKnown,
                              sizeof(kRollKnown) / sizeof(kRollKnown[0]), ignored, warnings);
    }
  }

  if (root.containsKey("pitchPid")) {
    if (!root["pitchPid"].is<JsonObject>()) {
      AddIgnored(ignored, warnings, "pitchPid", "expects an object");
    } else {
      JsonObjectConst pitch = root["pitchPid"].as<JsonObjectConst>();
      ApplyFloatField(pitch, "p", &merged.pitch_pid.kp, "pitchPid", applied, ignored, warnings);
      ApplyFloatField(pitch, "kp", &merged.pitch_pid.kp, "pitchPid", applied, ignored, warnings);
      ApplyFloatField(pitch, "i", &merged.pitch_pid.ki, "pitchPid", applied, ignored, warnings);
      ApplyFloatField(pitch, "ki", &merged.pitch_pid.ki, "pitchPid", applied, ignored, warnings);
      ApplyFloatField(pitch, "d", &merged.pitch_pid.kd, "pitchPid", applied, ignored, warnings);
      ApplyFloatField(pitch, "kd", &merged.pitch_pid.kd, "pitchPid", applied, ignored, warnings);
      static const char* kPitchKnown[] = {"p", "kp", "i", "ki", "d", "kd"};
      WarnUnknownNestedFields(pitch, "pitchPid", kPitchKnown,
                              sizeof(kPitchKnown) / sizeof(kPitchKnown[0]), ignored, warnings);
    }
  }

  if (root.containsKey("yawPid")) {
    if (!root["yawPid"].is<JsonObject>()) {
      AddIgnored(ignored, warnings, "yawPid", "expects an object");
    } else {
      JsonObjectConst yaw = root["yawPid"].as<JsonObjectConst>();
      ApplyFloatField(yaw, "p", &merged.yaw_pid.kp, "yawPid", applied, ignored, warnings);
      ApplyFloatField(yaw, "kp", &merged.yaw_pid.kp, "yawPid", applied, ignored, warnings);
      ApplyFloatField(yaw, "i", &merged.yaw_pid.ki, "yawPid", applied, ignored, warnings);
      ApplyFloatField(yaw, "ki", &merged.yaw_pid.ki, "yawPid", applied, ignored, warnings);
      ApplyFloatField(yaw, "d", &merged.yaw_pid.kd, "yawPid", applied, ignored, warnings);
      ApplyFloatField(yaw, "kd", &merged.yaw_pid.kd, "yawPid", applied, ignored, warnings);
      static const char* kYawKnown[] = {"p", "kp", "i", "ki", "d", "kd"};
      WarnUnknownNestedFields(yaw, "yawPid", kYawKnown,
                              sizeof(kYawKnown) / sizeof(kYawKnown[0]), ignored, warnings);
    }
  }

  if (applied.size() == 0) {
    response_doc["ok"] = false;
    response_doc["message"] = "No valid PID fields were provided";
    response_doc["hint"] =
        "Send nested rollPid/pitchPid/yawPid {p,i,d} or flat rollP..yawD numeric fields";
    SendJson(400, &response_doc);
    return;
  }

  String message;
  const bool ok = FlightController::updateSettings(merged, &message);
  response_doc["ok"] = ok;
  response_doc["saved"] = ok;
  response_doc["message"] = message;
  SendJson(ok ? 200 : 400, &response_doc);
}

void HandleCalibratePost() {
  if (RejectIfWebRxBlocked()) return;
  String message;
  const bool ok = FlightController::calibrateLevel(&message);
  SendResult(ok, message, ok ? 200 : 400);
}

void HandleReadOnlyCommand() {
  SendResult(false, "This command is disabled in this firmware profile.", 403);
}

}  // namespace

void WebUi::begin(const FlightSettings& settings) {
  WifiManager::beginAccessPoint(settings);

  if (g_server_started) return;

  g_server.on("/", HTTP_GET, HandleRoot);
  g_server.on("/api/state", HTTP_GET, HandleState);
  g_server.on("/api/state-heavy", HTTP_GET, HandleStateHeavy);
  g_server.on("/api/settings", HTTP_GET, HandleSettingsGet);
  g_server.on("/api/settings", HTTP_POST, HandleSettingsPost);
  g_server.on("/api/pid", HTTP_POST, HandlePidPost);
  g_server.on("/api/calibrate-level", HTTP_POST, HandleCalibratePost);
  g_server.on("/api/optimize-weights", HTTP_POST, HandleReadOnlyCommand);
  g_server.on("/api/home/set", HTTP_POST, []() {
    if (RejectIfWebRxBlocked()) return;
    String message;
    const bool ok = FlightController::setGpsHome(&message);
    SendResult(ok, message, ok ? 200 : 400);
  });
  g_server.on("/api/home/clear", HTTP_POST, []() {
    if (RejectIfWebRxBlocked()) return;
    FlightController::clearGpsHome();
    SendResult(true, "Home cleared", 200);
  });
  g_server.on("/api/disarm", HTTP_POST, HandleReadOnlyCommand);
  g_server.onNotFound([]() { SendResult(false, "Not found", 404); });
  g_server.begin();
  g_server_started = true;
}

void WebUi::loop() {
  if (g_network_reconfigure_pending &&
      static_cast<int32_t>(millis() - g_network_reconfigure_after_ms) >= 0) {
    WifiManager::reconfigureAccessPoint(g_network_reconfigure_settings);
    g_network_reconfigure_pending = false;
  }

  WifiManager::loop();
  g_server.handleClient();
}

void WebUi::reconfigureNetwork(const FlightSettings& settings) {
  WifiManager::reconfigureAccessPoint(settings);
}

void WebUi::scheduleReconfigureNetwork(const FlightSettings& settings) {
  g_network_reconfigure_settings = settings;
  g_network_reconfigure_after_ms = millis() + kDeferredNetworkReconfigureMs;
  g_network_reconfigure_pending = true;
}

String WebUi::ipAddress() {
  return WifiManager::ipAddress();
}
