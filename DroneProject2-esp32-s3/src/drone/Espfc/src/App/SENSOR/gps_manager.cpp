#include "SENSOR/gps_manager.h"

#include <TinyGPSPlus.h>

#include "INPUT/drone_config.h"

namespace {

// GPS hardware serial interface
// واجهة UART للعتاد GPS
// 中文: GPS硬件串行接口
HardwareSerial g_gps_serial(1);
TinyGPSPlus g_gps;  // GPS parser / محلل GPS / GPS解析器

// GPS state variables
// متغيرات حالة GPS
// 中文: GPS状态变量
GpsSnapshot g_snapshot{};              // Current GPS data / بيانات GPS الحالية / 当前GPS数据
bool g_enabled = true;                 // GPS enabled flag / علم تفعيل GPS / GPS启用标志
bool g_home_set = false;               // Home position set / تعيين موضع المنزل / 家位置已设置
double g_home_latitude = 0.0;          // Home latitude / خط عرض المنزل / 家纬度
double g_home_longitude = 0.0;         // Home longitude / خط طول المنزل / 家经度
uint32_t g_last_stream_ms = 0;         // Last data received / آخر بيانات مستقبلة / 上次接收数据
uint32_t g_current_baud = DroneConfig::kGpsBaud;  // Current baud rate / معدل البت الحالي / 当前波特率
uint32_t g_last_probe_ms = 0;          // Last baud probe / آخر اختبار للبت / 上次波特率探测
bool g_stream_locked = false;          // Stream locked flag / علم قفل التدفق / 数据流锁定标志
bool g_reported_stream = false;        // Reported lock status / حالة قفل المبلغة / 已报告锁定状态
char g_nmea_line[128] = {};            // NMEA buffer / مخزن مؤقت NMEA / NMEA缓冲区
uint8_t g_nmea_length = 0;             // Buffer length / طول المخزن / 缓冲区长度

// GPS health thresholds
// عتبات صحة GPS
// 中文: GPS健康阈值
constexpr uint32_t kFixMaxAgeMs = 2000;        // Max fix age / أقصى عمر للإصلاح / 最大定位时效
constexpr uint32_t kHealthyTimeoutMs = 5000;     // Health timeout / مهلة الصحة / 健康超时
constexpr uint32_t kMinFixSatellites = 5;      // Min satellites for fix / أقل أقمار للإصلاح / 定位所需最少卫星数
constexpr uint32_t kProbePeriodMs = 1800;      // Baud probe interval / فترة اختبار البت / 波特率探测间隔
constexpr uint32_t kGpsBaudCandidates[] = {DroneConfig::kGpsBaud, 9600, 38400, 57600, 115200};

// Validate NMEA sentence format
// التحقق من تنسيق جملة NMEA
// 中文: 验证NMEA语句格式
bool IsLikelyNmeaSentence(const char* line) {
  if (line == nullptr) return false;
  if (line[0] != '$') return false;
  if (line[1] != 'G') return false;
  return (strchr(line, ',') != nullptr) && (strchr(line, '*') != nullptr);
}

// Reset NMEA buffer
// إعادة تعيين مخزن NMEA
// 中文: 重置NMEA缓冲区
void ResetNmeaLine() {
  g_nmea_length = 0;
  g_nmea_line[0] = '\0';
}

// Initialize GPS serial at specified baud rate
// تهيئة UART GPS بمعدل البت المحدد
// 中文: 以指定波特率初始化GPS串口
void BeginSerialAt(uint32_t baud) {
  g_gps_serial.end();
  g_gps_serial.begin(baud, SERIAL_8N1, DroneConfig::kGpsRxPin, DroneConfig::kGpsTxPin);
  g_current_baud = baud;
  g_last_probe_ms = millis();
  ResetNmeaLine();
}

void AdvanceProbeBaud() {
  size_t current_index = 0;
  for (size_t i = 0; i < (sizeof(kGpsBaudCandidates) / sizeof(kGpsBaudCandidates[0])); ++i) {
    if (kGpsBaudCandidates[i] == g_current_baud) {
      current_index = i;
      break;
    }
  }
  const size_t next_index =
      (current_index + 1) % (sizeof(kGpsBaudCandidates) / sizeof(kGpsBaudCandidates[0]));
  BeginSerialAt(kGpsBaudCandidates[next_index]);
  Serial.print(F("GPS probe: trying "));
  Serial.print(g_current_baud);
  Serial.println(F(" baud"));
}

void ProcessNmeaByte(char c) {
  if (c == '\r') return;

  if (c == '\n') {
    g_nmea_line[g_nmea_length] = '\0';
    if (IsLikelyNmeaSentence(g_nmea_line)) {
      g_last_stream_ms = millis();
      if (!g_stream_locked) {
        g_stream_locked = true;
        Serial.print(F("GPS stream detected at "));
        Serial.print(g_current_baud);
        Serial.println(F(" baud"));
      }
      g_reported_stream = true;
    }
    ResetNmeaLine();
    return;
  }

  if (g_nmea_length >= (sizeof(g_nmea_line) - 1)) {
    ResetNmeaLine();
    return;
  }

  if (g_nmea_length == 0 && c != '$') {
    return;
  }

  g_nmea_line[g_nmea_length++] = c;
}

bool IsFixValid() {
  if (!g_enabled) return false;
  if (!g_gps.location.isValid()) return false;
  if (g_gps.location.age() > kFixMaxAgeMs) return false;
  if (!g_gps.satellites.isValid()) return false;
  return g_gps.satellites.value() >= kMinFixSatellites;
}

float ReadHdop() {
  if (!g_gps.hdop.isValid()) return 99.9f;
  return static_cast<float>(g_gps.hdop.hdop());
}

void UpdateSnapshotFromParser() {
  const uint32_t now_ms = millis();
  const bool fix_valid = IsFixValid();

  g_snapshot.fix = fix_valid;
  g_snapshot.home_set = g_home_set;
  g_snapshot.age_ms = g_gps.location.isValid() ? g_gps.location.age() : 0;
  g_snapshot.satellites = g_gps.satellites.isValid() ? g_gps.satellites.value() : 0;
  g_snapshot.hdop = ReadHdop();
  g_snapshot.speed_mps = g_gps.speed.isValid() ? static_cast<float>(g_gps.speed.mps()) : 0.0f;
  g_snapshot.altitude_m =
      g_gps.altitude.isValid() ? static_cast<float>(g_gps.altitude.meters()) : 0.0f;
  g_snapshot.course_deg =
      g_gps.course.isValid() ? static_cast<float>(g_gps.course.deg()) : 0.0f;

  if (fix_valid) {
    g_snapshot.latitude = g_gps.location.lat();
    g_snapshot.longitude = g_gps.location.lng();
  } else {
    g_snapshot.latitude = 0.0;
    g_snapshot.longitude = 0.0;
  }

  g_snapshot.home_latitude = g_home_set ? g_home_latitude : 0.0;
  g_snapshot.home_longitude = g_home_set ? g_home_longitude : 0.0;

  if (fix_valid && g_home_set) {
    g_snapshot.distance_home_m = static_cast<float>(
        TinyGPSPlus::distanceBetween(g_snapshot.latitude, g_snapshot.longitude, g_home_latitude,
                                     g_home_longitude));
    g_snapshot.bearing_home_deg = static_cast<float>(
        TinyGPSPlus::courseTo(g_snapshot.latitude, g_snapshot.longitude, g_home_latitude,
                              g_home_longitude));
  } else {
    g_snapshot.distance_home_m = 0.0f;
    g_snapshot.bearing_home_deg = 0.0f;
  }

  const bool stream_recent =
      (g_last_stream_ms != 0) && ((now_ms - g_last_stream_ms) <= kHealthyTimeoutMs);
  g_snapshot.healthy = g_enabled && stream_recent;
}

}  // namespace

void GpsManager::begin() {
  g_snapshot = {};
  g_enabled = true;
  g_home_set = false;
  g_home_latitude = 0.0;
  g_home_longitude = 0.0;
  g_last_stream_ms = 0;
  g_current_baud = DroneConfig::kGpsBaud;
  g_last_probe_ms = 0;
  g_stream_locked = false;
  g_reported_stream = false;
  ResetNmeaLine();

  BeginSerialAt(DroneConfig::kGpsBaud);
  Serial.print(F("GPS probe: trying "));
  Serial.print(g_current_baud);
  Serial.println(F(" baud"));
}

void GpsManager::setEnabled(bool enabled) {
  g_enabled = enabled;
  if (!g_enabled) {
    g_snapshot.healthy = false;
    g_snapshot.fix = false;
    g_snapshot.speed_mps = 0.0f;
    g_snapshot.distance_home_m = 0.0f;
    g_snapshot.bearing_home_deg = 0.0f;
  }
}

bool GpsManager::isEnabled() {
  return g_enabled;
}

void GpsManager::update() {
  if (g_enabled) {
    while (g_gps_serial.available() > 0) {
      const char c = static_cast<char>(g_gps_serial.read());
      g_gps.encode(c);
      ProcessNmeaByte(c);
    }

    if (!g_stream_locked && (millis() - g_last_probe_ms) >= kProbePeriodMs) {
      AdvanceProbeBaud();
    }

    const bool stream_recent =
        (g_last_stream_ms != 0) && ((millis() - g_last_stream_ms) <= kHealthyTimeoutMs);
    if (g_stream_locked && !stream_recent) {
      g_stream_locked = false;
      if (g_reported_stream) {
        Serial.println(F("GPS stream timeout, resuming baud probe"));
        g_reported_stream = false;
      }
    }
  }
  UpdateSnapshotFromParser();
}

GpsSnapshot GpsManager::snapshot() {
  return g_snapshot;
}

bool GpsManager::setHomeToCurrent() {
  if (!g_snapshot.fix) return false;
  g_home_latitude = g_snapshot.latitude;
  g_home_longitude = g_snapshot.longitude;
  g_home_set = true;
  g_snapshot.home_set = true;
  g_snapshot.home_latitude = g_home_latitude;
  g_snapshot.home_longitude = g_home_longitude;
  return true;
}

void GpsManager::clearHome() {
  g_home_set = false;
  g_home_latitude = 0.0;
  g_home_longitude = 0.0;
  g_snapshot.home_set = false;
  g_snapshot.home_latitude = 0.0;
  g_snapshot.home_longitude = 0.0;
  g_snapshot.distance_home_m = 0.0f;
  g_snapshot.bearing_home_deg = 0.0f;
}
