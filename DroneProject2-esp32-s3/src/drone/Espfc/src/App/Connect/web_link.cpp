#include "Connect/web_link.h"

#include "INPUT/drone_config.h"

namespace WebLink {

// Decode web link mode from CH8 (AUX2) RC channel
// فك تشفير وضع رابط الويب من قناة RC CH8 (AUX2)
// 中文: 从CH8 (AUX2) RC通道解码网页链接模式
uint8_t DecodeMode(const RcSnapshot& rc) {
  // Return normal mode if feature is disabled
  // إرجاع الوضع العادي إذا كانت الميزة معطلة
  // 中文: 如果功能被禁用，返回正常模式
  if (!DroneConfig::kEnableCh8WebLinkGate) {
    return DroneConfig::kWebLinkModeNormal;
  }

  // Return normal mode if AUX2 signal is stale
  // إرجاع الوضع العادي إذا كانت إشارة AUX2 قديمة
  // 中文: 如果AUX2信号已过期，返回正常模式
  if (!rc.fresh[DroneConfig::kRcAux2Index]) {
    return DroneConfig::kWebLinkModeNormal;
  }

  // Decode mode based on AUX2 pulse width
  // فك تشفير الوضع بناءً على عرض نبضة AUX2
  // 中文: 根据AUX2脉宽解码模式
  const uint16_t aux2 = rc.channels[DroneConfig::kRcAux2Index];
  if (aux2 <= DroneConfig::kWebLinkLowMaxUs) {
    return DroneConfig::kWebLinkModeNormal;        // Low = normal mode / منخفض = الوضع العادي / 低电平=正常模式
  }
  if (aux2 >= DroneConfig::kWebLinkHighMinUs) {
    return DroneConfig::kWebLinkModeReceivePriority; // High = receive priority / مرتفع = أولوية الاستقبال / 高电平=接收优先
  }
  return DroneConfig::kWebLinkModeBroadcastOnly;    // Mid = broadcast only / متوسط = البث فقط / 中值=仅广播
}

// Check if command reception is allowed in current mode
// التحقق مما إذا كان استقبال الأوامر مسموحاً في الوضع الحالي
// 中文: 检查当前模式下是否允许接收命令
bool RxAllowed(uint8_t mode) {
  return mode != DroneConfig::kWebLinkModeBroadcastOnly;
}

// Get polling interval hint based on mode
// الحصول على تلميح فترة الاستقصاء بناءً على الوضع
// 中文: 根据模式获取轮询间隔提示
uint16_t PollHintMs(uint8_t mode) {
  return mode == DroneConfig::kWebLinkModeReceivePriority
             ? DroneConfig::kWebPollReceivePriorityMs    // Slower polling in priority mode / استقصاء أبطأ في وضع الأولوية / 优先模式下轮询更慢
             : DroneConfig::kWebPollNormalMs;            // Normal polling interval / فترة الاستقصاء العادية / 正常轮询间隔
}

// Get heavy update interval hint based on mode
// الحصول على تلميح فترة التحديث الثقيل بناءً على الوضع
// 中文: 根据模式获取重更新间隔提示
uint16_t HeavyUpdateHintMs(uint8_t mode) {
  return mode == DroneConfig::kWebLinkModeReceivePriority
             ? DroneConfig::kWebHeavyUpdateReceivePriorityMs  // Slower updates in priority mode / تحديثات أبطأ في وضع الأولوية / 优先模式下更新更慢
             : DroneConfig::kWebHeavyUpdateNormalMs;         // Normal update interval / فترة التحديث العادية / 正常更新间隔
}

// Get human-readable mode name
// الحصول على اسم الوضع المقروء للبشر
// 中文: 获取人类可读的模式名称
const char* ModeName(uint8_t mode) {
  switch (mode) {
    case DroneConfig::kWebLinkModeBroadcastOnly:
      return "broadcast";      // Broadcast only mode / وضع البث فقط / 仅广播模式
    case DroneConfig::kWebLinkModeReceivePriority:
      return "receive";        // Receive priority mode / وضع أولوية الاستقبال / 接收优先模式
    case DroneConfig::kWebLinkModeNormal:
    default:
      return "normal";         // Normal mode / الوضع العادي / 正常模式
  }
}

}  // namespace WebLink
