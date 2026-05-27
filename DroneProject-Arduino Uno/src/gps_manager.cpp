#include <Arduino.h>
#include <TinyGPSPlus.h>
#include "controller_config.h"
#include "gps_manager.h"

namespace {

// ============================================================================
// 基于 TinyGPSPlus 库的 GPS 数据解析器
// ============================================================================
// 该对象用于收集串口读取的字符并解码 NMEA 语句。
// 不在堆 (heap) 上分配内存，以减少 Uno 开发板的内存占用。
TinyGPSPlus gps;

// ============================================================================
// 运行状态变量
// ============================================================================
bool enabled = false;          // GPS 处理器是否已启用？
bool homeIsSet = false;        // 是否已确定返航点 (Home) 位置？
double homeLatitude = 0.0;     // 返航点纬度
double homeLongitude = 0.0;    // 返航点经度

}  // 命名空间 (namespace)

void GpsManager::begin() {
  // ============================================================================
  // 初始化 GPS 传感器串口通信
  // ============================================================================
  // GPS 连接至硬件串口
  GPS_SERIAL.begin(Config::GPS_BAUD);
}

bool GpsManager::isHealthy() {
  // ============================================================================
  // 检查 GPS 连接状态
  // ============================================================================
  // 一旦启用，检查解析器是否已接收到字符。
  // 这可以快速判断硬件是否已连接并正在发送数据，
  // 但不保证已获取有效的定位 (fix)。
  return gps.charsProcessed() > 0;
}

void GpsManager::setEnabled(bool newEnabled) {
  // ============================================================================
  // 启用/禁用 GPS 处理器
  // ============================================================================
  if (newEnabled == enabled) return;  // 无需更改

  enabled = newEnabled;
  if (enabled) {
    homeIsSet = false;  // 重新启用时清除旧的返航点位置
  }
}

bool GpsManager::isEnabled() {
  return enabled;
}

// ============================================================================
// 更新 GPS 数据并捕捉返航点 (Home) 位置
// ============================================================================
// 当传感器组合开关开启时，从 FlightController::loop() 主循环调用。
// 禁用时避免轮询，以防止因 GPS 通信不良导致处理器负载过高。
void GpsManager::update() {
  if (!enabled) return;

  while (GPS_SERIAL.available()) {
    gps.encode(GPS_SERIAL.read());
  }

  // 一旦在启用后获得有效的定位数据，将其记录为“返航点”坐标。
  // 随后的位置更新不会覆盖返航点。
  if (!homeIsSet && gps.location.isValid() && gps.location.age() < 2000) {
    homeLatitude = gps.location.lat();
    homeLongitude = gps.location.lng();
    homeIsSet = true;
  }

#if GPS_DEBUG
  static uint32_t lastPrintMs = 0;
  if (millis() - lastPrintMs > 1000) {
    lastPrintMs = millis();
    if (gps.location.isValid()) {
      Serial.print(F("GPS Lat: "));
      Serial.print(gps.location.lat(), 6);
      Serial.print(F(" Lng: "));
      Serial.print(gps.location.lng(), 6);
      Serial.print(F(" Sats: "));
      Serial.println(gps.satellites.value());
    } else {
      Serial.println(F("GPS waiting for fix..."));
    }
  }
#endif
}

// 返航点读取接口 (用于未来的导航功能)
bool GpsManager::homeSet() {
  return homeIsSet;
}

double GpsManager::homeLat() {
  return homeLatitude;
}

double GpsManager::homeLng() {
  return homeLongitude;
}