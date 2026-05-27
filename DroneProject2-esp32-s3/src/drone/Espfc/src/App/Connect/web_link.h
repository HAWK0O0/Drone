#pragma once

// Web Link Control Module
// وحدة تحكم رابط الويب
// 中文: 网页链接控制模块
// Manages communication modes based on CH8 switch input
// تدير أوضاع الاتصال بناءً على مدخل مفتاح CH8
// 中文: 基于 CH8 开关输入管理通信模式

#include "INPUT/drone_types.h"

namespace WebLink {

uint8_t DecodeMode(const RcSnapshot& rc);
bool RxAllowed(uint8_t mode);
uint16_t PollHintMs(uint8_t mode);
uint16_t HeavyUpdateHintMs(uint8_t mode);
const char* ModeName(uint8_t mode);

}  // namespace WebLink
