#pragma once

#include <Arduino.h>

// Main flight loop and safe command entry points.
// الحلقة الرئيسية للطيران ونقاط الأوامر الآمنة.
// 中文: 主飞控循环与安全控制入口。

#include "INPUT/drone_types.h"

namespace FlightController {

void setup();
void loop();

TelemetrySnapshot telemetry();
FlightSettings settings();

bool updateSettings(const FlightSettings& settings, String* message);
bool calibrateLevel(String* message);
bool optimizeWeights(String* message);
bool setGpsHome(String* message);
void clearGpsHome();
void disarm();

}  // namespace FlightController
