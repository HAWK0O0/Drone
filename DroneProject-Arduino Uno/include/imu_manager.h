#pragma once

// ============================================================================
// IMU(惯性测量单元)管理器
// 负责读取和处理陀螺仪、加速度计和磁力计数据
// 计算无人机的欧拉角(翻滚、俯仰、偏航)
// ============================================================================
namespace ImuManager {

// 初始化IMU传感器
// 检测传感器是否连接，执行校准
void begin();

// 更新IMU数据
// 读取新的传感器值，计算当前姿态
void update(bool autoLevelEnabled);  // 是否启用自动平衡

// 检查IMU是否就绪可用
bool isReady();

// 检查IMU校准是否成功
bool calibrationOk();

// 获取当前翻滚角(度) - 绕X轴旋转
float roll();

// 获取当前俯仰角(度) - 绕Y轴旋转
float pitch();

// 获取当前偏航角(度) - 绕Z轴旋转
float yaw();

}  // namespace ImuManager
