#pragma once

// Web UI Module
// وحدة واجهة الويب
// 中文: Web UI 模块
// Handles web interface HTTP server and dashboard
// تدير خادم HTTP ولوحة تحكم واجهة الويب
// 中文: 管理 Web 界面 HTTP 服务器和仪表板

#include <Arduino.h>

#include "INPUT/drone_types.h"

namespace WebUi {

void begin(const FlightSettings& settings);
void loop();
void reconfigureNetwork(const FlightSettings& settings);
void scheduleReconfigureNetwork(const FlightSettings& settings);
String ipAddress();

}  // namespace WebUi
