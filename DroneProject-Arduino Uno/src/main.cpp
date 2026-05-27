#include <Arduino.h>
#include "flight_controller.h"

// ============================================================================
// 无人机飞行控制系统 - 主程序入口
// ============================================================================
// 注意: 保持main.cpp简单稳定
// 所有飞行逻辑都放在FlightController和各个模块中

// 初始化函数 - Arduino启动时执行一次
// 调用飞行控制器的setup函数初始化所有系统
void setup() {
  FlightController::setup();
}

// 主循环函数 - 不断循环执行
// 读取传感器、处理遥控输入、计算电机输出
void loop() {
  FlightController::loop();
}

