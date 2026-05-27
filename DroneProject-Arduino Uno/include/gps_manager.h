#pragma once

// ============================================================================
// GPS (全球定位系统) 管理器
// 读取GPS模块的位置和速度信息
// 用于自动返航(Return to Home)功能
// ============================================================================
namespace GpsManager {

// 初始化GPS模块(UART通信)
void begin();

// 启用或禁用GPS数据处理
void setEnabled(bool enabled);

// 检查GPS是否启用
bool isEnabled();

// 更新GPS数据
// 读取GPS模块的数据并解析
void update();

// 检查GPS模块是否正常工作
// 返回true表示已收到有效的NMEA数据
bool isHealthy();

// 检查是否设置了返航点(Home位置)
bool homeSet();

// 获取返航点纬度(度)
double homeLat();

// 获取返航点经度(度)
double homeLng();

}  // namespace GpsManager
