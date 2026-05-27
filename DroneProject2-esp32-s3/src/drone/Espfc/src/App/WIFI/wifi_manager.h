#pragma once

// Wi-Fi Manager Module
// وحدة إدارة Wi-Fi
// 中文: Wi-Fi 管理器模块
// Manages AP/STA Wi-Fi connections and network configuration
// تدير اتصالات Wi-Fi AP/STA وإعدادات الشبكة
// 中文: 管理 AP/STA Wi-Fi 连接和网络配置

#include <Arduino.h>

#include "INPUT/drone_types.h"

namespace WifiManager {

void beginAccessPoint(const FlightSettings& settings);
void reconfigureAccessPoint(const FlightSettings& settings);
void loop();
String accessPointIpAddress();
String stationIpAddress();
String ipAddress();
bool stationEnabled();
bool stationConnected();
int32_t stationRssi();

}  // namespace WifiManager
