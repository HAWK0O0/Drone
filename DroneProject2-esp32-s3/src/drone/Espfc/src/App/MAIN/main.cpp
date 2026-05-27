#include <Arduino.h>
#include "Control/flight_controller.h"

void setup() {
  // Initialize all drone systems and start flight controller
  // تهيئة جميع أنظمة الطائرة وبدء FlightController
  // 中文: 初始化所有无人机系统并启动飞控
  FlightController::setup();
}

void loop() {
  // Main flight control loop - runs continuously
  // الحلقة الرئيسية للتحكم في الطيران - تعمل باستمرار
  // 中文: 主飞控循环 - 持续运行
  FlightController::loop();
}
