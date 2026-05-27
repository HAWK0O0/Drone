/*
 * ESP32-S3 Drone Configuration
 * 
 * Main configuration file for drone hardware and system settings
 * ملف الإعدادات الرئيسي لأجهزة وإعدادات الطائرة
 * 中文: 无人机硬件和系统设置的主配置文件
 * 
 * This file contains all hardware pin definitions, communication settings,
 * and system constants used throughout the drone firmware.
 * هذا الملف يحتوي على تعريفات دبابيس الأجهزة، إعدادات الاتصال،
 * والثوابت النظامية المستخدمة في فيرموير الطائرة.
 * 此文件包含无人机固件中使用的所有硬件引脚定义、通信设置和系统常量。
 */

#pragma once

#include <Arduino.h>

namespace DroneConfig {

// Firmware identification and version
// تعريف الفيرموير والإصدار
// 中文: 固件标识和版本
constexpr char kFirmwareName[] = "ESP32-S3 Drone FC";  // Firmware name / اسم الفيرموير / 固件名称
constexpr uint32_t kSettingsVersion = 5;               // Settings version / إصدار الإعدادات / 设置版本

// Safety warnings - CRITICAL for this hardware configuration
// تحذيرات الأمان - حاسمة لهذا التكوين العتادي  
// 中文: 安全警告 - 此硬件配置的关键信息
// Keep the camera ribbon disconnected, keep the SD slot empty,
// and always build with PSRAM disabled for this board wiring.
// اترك شريط الكاميرا مفصولا، ولا تدخل بطاقة الذاكرة،
// وعطّل PSRAM دائما مع هذا التوصيل.
// 摄像头排线必须断开，SD 卡槽必须留空，并始终关闭 PSRAM。

// Motor pin assignments (M1-M4)
// تعيين دبابيس المحركات (M1-M4)
// 中文: 电机引脚分配 (M1-M4)
// Motor layout: M1 front-left, M2 front-right, M3 rear-left, M4 rear-right.
// تخطيط المحركات: M1 أمامي يسار، M2 أمامي يمين، M3 خلفي يسار، M4 خلفي يمين.
// 电机布局：M1 左前，M2 右前，M3 左后，M4 右后。
constexpr uint8_t kMotorPins[4] = {38, 39, 40, 37};  // Motor GPIO pins / دبابيس GPIO للمحركات / 电机 GPIO 引脚

// Receiver interface: single-wire SBUS on GPIO9.
// واجهة المستقبل: SBUS بسلك إشارة واحد على GPIO9.
// 接收机接口：GPIO9 上的单线 SBUS。
constexpr char kRcProtocol[] = "SBUS";
constexpr uint8_t kRcSbusRxPin = 18;         // SBUS RX pin / دبوس RX لإشارة SBUS / SBUS RX 引脚
constexpr uint32_t kRcSbusBaud = 100000;    // SBUS baud rate / معدل SBUS / SBUS 波特率
constexpr bool kRcSbusInvert = true;        // Standard SBUS uses inverted logic / SBUS القياسي معكوس / 标准 SBUS 为反相逻辑
constexpr uint8_t kRcSbusUart = 2;          // Dedicated UART index / رقم UART المخصص / 专用 UART 编号

// Receiver channel mapping
// تخطيط قنوات المستقبل  
// 中文: 接收机通道映射
// CH1 = Roll, CH2 = Pitch, CH3 = Throttle, CH4 = Yaw.
constexpr uint8_t kRcRollIndex = 0;      // Roll channel / قناة اليمين/يسار / 横滚通道
constexpr uint8_t kRcPitchIndex = 1;     // Pitch channel / قناة أمام/خلف / 俯仰通道
constexpr uint8_t kRcThrottleIndex = 2;  // Throttle channel / قناة الخنق / 油门通道
constexpr uint8_t kRcYawIndex = 3;       // Yaw channel / قناة الانعراج / 偏航通道
constexpr uint8_t kRcArmIndex = 4;       // Arm switch channel on CH5 / قناة مفتاح التسليح على CH5 / CH5 解锁开关通道
constexpr uint8_t kRcSensorIndex = 5;     // Sensor/stabilize switch channel on CH6 / قناة مفتاح المستشعرات على CH6 / CH6 传感器/平衡开关通道
constexpr uint8_t kRcAux1Index = 6;      // Auxiliary channel 1 on CH7 / قناة إضافية 1 على CH7 / CH7 辅助通道 1
constexpr uint8_t kRcAux2Index = 7;      // Auxiliary channel 2 / قناة إضافية 2 / 辅助通道 2

// Axis direction corrections
// تصحيحات اتجاه المحاور
// 中文: 轴向方向修正
// Roll and yaw follow transmitter directly.
// Pitch is inverted so CH2 high (2000) commands forward motion.
constexpr int kRcRollDirection = 1;   // Roll direction (normal) / اتجاه اليمين/يسار (عادي) / 横滚方向（正常）
constexpr int kRcPitchDirection = -1;  // Pitch direction (inverted) / اتجاه أمام/خلف (معكوس) / 俯仰方向（反向）
constexpr int kRcYawDirection = 1;    // Yaw direction (normal) / اتجاه الانعراج (عادي) / 偏航方向（正常）

constexpr uint8_t kI2cSclPin = 10;  // I2C SCL pin / دبوس SCL ل I2C / I2C SCL 引脚
constexpr uint8_t kI2cSdaPin = 11;  // I2C SDA pin / دبوس SDA ل I2C / I2C SDA 引脚

constexpr uint8_t kGpsRxPin = 16;  // GPS RX pin / دبوس RX لل GPS / GPS RX 引脚
constexpr uint8_t kGpsTxPin = 17;  // GPS TX pin / دبوس TX لل GPS / GPS TX 引脚
constexpr uint32_t kGpsBaud = 115200;  // GPS baud rate / معدل البت لل GPS / GPS 波特率

// TFT Display Pins
// دبابيس شاشة TFT
// 中文: TFT 显示屏引脚
constexpr int8_t kTftRstPin = -1;         // TFT reset pin (-1 = not wired) / دبوس إعادة تعيين TFT (-1 = غير موصول) / TFT 复位引脚（-1 表示未连接）
constexpr int8_t kTftDcPin = -1;          // TFT data/command pin / دبوس البيانات/الأوامر / TFT 数据/命令引脚
constexpr int8_t kTftMosiPin = -1;        // TFT MOSI pin / دبوس MOSI / TFT MOSI 引脚
constexpr int8_t kTftSckPin = -1;         // TFT clock pin / دبوس الساعة / TFT 时钟引脚
constexpr int8_t kTftCsPin = -1;          // TFT chip select pin / دبوس اختيار الشريحة / TFT 片选引脚
constexpr int8_t kTftBacklightPin = -1;   // TFT backlight pin / دبوس الإضاءة الخلفية / TFT 背光引脚
// TFT Display Settings
// إعدادات شاشة TFT
// 中文: TFT 显示屏设置
constexpr bool kEnableTft = false;       // Enable TFT display / تفعيل شاشة TFT / 启用 TFT 显示屏
constexpr uint16_t kTftWidth = 240;      // TFT screen width / عرض شاشة TFT / TFT 屏幕宽度
constexpr uint16_t kTftHeight = 240;     // TFT screen height / ارتفاع شاشة TFT / TFT 屏幕高度;
constexpr uint8_t kTftRotation = 0;      // TFT rotation 0..3 / اتجاه شاشة TFT من 0 إلى 3 / TFT 旋转方向 0..3

constexpr uint16_t kRcMinUs = 1000;        // Minimum normalized RC pulse / أقل نبضة RC معيارية / 最小标准化 RC 脉宽
constexpr uint16_t kRcMidUs = 1500;        // Center RC pulse / عرض نبضة RC المركز / 中心 RC 脉宽
constexpr uint16_t kRcMaxUs = 2000;        // Maximum normalized RC pulse / أكبر نبضة RC معيارية / 最大标准化 RC 脉宽
constexpr uint16_t kRcPulseMinUs = 900;      // Absolute minimum accepted RC pulse / أقل نبضة RC مقبولة / 绝对最小接收 RC 脉宽
constexpr uint16_t kRcPulseMaxUs = 2200;     // Absolute maximum accepted RC pulse / أكبر نبضة RC مقبولة / 绝对最大接收 RC 脉宽
constexpr uint16_t kRcSwitchOffUs = 1300;    // Switch OFF threshold / عتبة إيقاف المفتاح / 开关关闭阈值
constexpr uint16_t kRcSwitchOnUs = 1700;     // Switch ON threshold / عتبة تشغيل المفتاح / 开关开启阈值
constexpr uint32_t kRcFailsafeUs = 300000;  // Failsafe timeout (microseconds) / مهلة فقدان الإشارة (ميكروثانية) / 失控超时（微秒）
constexpr uint16_t kRcDeadbandUs = 25;       // Stick deadband / نطاق ميت للعصا / 摇杆死区
constexpr uint16_t kRcEndpointSnapUs = 5;     // Endpoint snap threshold / عتبة الالتزام بالنهاية / 终点吸附阈值
constexpr float kYawRateMaxDps = 140.0f;     // Max yaw rate target / أقصى سرعة انعراج مطلوبة / 最大偏航目标速率

// CH8 Web Link Control - Critical for network behavior
// تحكم رابط ويب CH8 - حاسم لسلوك الشبكة
// 中文: CH8 网页链接控制 - 网络行为的关键
constexpr bool kEnableCh8WebLinkGate = true;           // Enable CH8 link control / تفعيل تحكم رابط CH8 / 启用 CH8 链路控制
constexpr uint16_t kWebLinkLowMaxUs = 1250;           // AP mode threshold (<1250) / عتبة وضع AP (<1250) / AP 模式阈值（<1250）
constexpr uint16_t kWebLinkHighMinUs = 1750;          // STA mode threshold (>1750) / عتبة وضع STA (>1750) / STA 模式阈值（>1750）
constexpr uint8_t kWebLinkModeNormal = 0;              // Normal mode / الوضع الطبيعي / 正常模式
constexpr uint8_t kWebLinkModeBroadcastOnly = 1;        // Broadcast only mode / وضع البث فقط / 仅广播模式
constexpr uint8_t kWebLinkModeReceivePriority = 2;        // Receive priority mode / وضع أولوية الاستقبال / 接收优先模式
constexpr uint16_t kWebPollNormalMs = 450;             // Normal polling interval / فترة الاستقصاء العادية / 正常轮询间隔
constexpr uint16_t kWebPollReceivePriorityMs = 1000;    // Priority polling interval / فترة الاستقصاء ذات الأولوية / 优先轮询间隔
constexpr uint16_t kWebHeavyUpdateNormalMs = 700;       // Normal update interval / فترة التحديث العادية / 正常更新间隔
constexpr uint16_t kWebHeavyUpdateReceivePriorityMs = 1500; // Priority update interval / فترة التحديث ذات الأولوية / 优先更新间隔
constexpr uint16_t kWebServicePeriodMs = 20;            // Internal HTTP service cadence / وتيرة خدمة HTTP الداخلية / 内部 HTTP 服务周期

// Motor Control Settings
// إعدادات تحكم المحركات
// 中文: 电机控制设置
constexpr uint16_t kEscMinUs = 1000;        // Minimum ESC pulse / أقل نبضة ESC / 最小 ESC 脉宽
constexpr uint16_t kEscMaxUs = 2000;        // Maximum ESC pulse / أكبر نبضة ESC / 最大 ESC 脉宽
constexpr uint16_t kMotorPwmHz = 400;       // PWM frequency in Hz / تردد PWM بالهرتز / PWM 频率（赫兹）
// ESP32-S3 LEDC maximum timer width is 14 bits.
constexpr uint8_t kMotorPwmBits = 14;       // PWM resolution bits / دقة PWM بالبت / PWM 分辨率位数
constexpr uint16_t kMotorOutputSlewUs = 260;  // Motor output slew rate / معدل تغيير خرج المحرك / 电机输出压摆率

// Flight Control Timing
// توقيت التحكم في الطيران
// 中文: 飞行控制时序
constexpr uint32_t kControlPeriodUs = 10000;     // Main control loop period / فترة حلقة التحكم الرئيسية / 主控制循环周期
constexpr uint32_t kImuReadPeriodUs = 10000;     // IMU sensor read period / فترة قراءة مستشعر IMU / IMU 传感器读取周期
constexpr uint32_t kGpsRefreshPeriodUs = 25000;  // GPS data refresh period / فترة تحديث بيانات GPS / GPS 数据刷新周期
constexpr uint32_t kTftRefreshMs = 500;         // TFT screen refresh period / فترة تحديث شاشة TFT / TFT 屏幕刷新周期
constexpr uint32_t kTftRefreshArmedMs = 1500;   // Slower TFT refresh while armed / تحديث أبطأ للشاشة أثناء التسليح / 解锁时更慢的 TFT 刷新周期

// IMU Sensor Configuration
// إعدادات مستشعر IMU
// 中文: IMU 传感器配置
constexpr bool kPreferMpu6050 = true;         // Prefer MPU6050 when both sensors are present / تفضيل MPU6050 عند وجود أكثر من مستشعر / 存在多个传感器时优先使用 MPU6050
constexpr uint8_t kMpu6050AddressA = 0x68;    // Primary MPU6050 I2C address / عنوان MPU6050 الأساسي / MPU6050 主 I2C 地址
constexpr uint8_t kMpu6050AddressB = 0x69;    // Secondary MPU6050 I2C address / عنوان MPU6050 الثانوي / MPU6050 备用 I2C 地址
constexpr uint16_t kMpu6050CalibrationSamples = 320;  // Startup bias samples / عدد عينات تعويض الانحياز عند الإقلاع / 启动偏置采样数
constexpr uint16_t kMpu6050CalibrationDelayMs = 4;    // Delay between startup samples / التأخير بين عينات الإقلاع / 启动采样间隔
constexpr float kMpu6050StillGyroThresholdDps = 5.0f; // Max gyro drift allowed during startup calibration / أكبر انحراف مسموح للجيروسكوب أثناء معايرة الإقلاع / 启动校准时允许的最大陀螺漂移
constexpr uint8_t kBnoAddressA = 0x28;        // First BNO055 I2C address / عنوان BNO055 الأول ل I2C / 第一个 BNO055 I2C 地址
constexpr uint8_t kBnoAddressB = 0x29;        // Second BNO055 I2C address / عنوان BNO055 الثاني ل I2C / 第二个 BNO055 I2C 地址
constexpr bool kBnoUseExternalCrystal = false; // Enable only if the module crystal is actually soldered / فعّلها فقط إذا كانت البلورة الخارجية ملحومة / 仅当外部晶振已焊接时启用
constexpr float kImuFilterAlpha = 0.98f;      // Complementary filter gyro weight for MPU6050 / وزن الجيروسكوب في الفلتر التكميلي لـ MPU6050 / MPU6050 互补滤波中的陀螺权重
constexpr int kRollSign = -1;                 // Roll axis sign correction / تصحيح إشارة محور اليمين/يسار / 横滚轴符号修正
constexpr int kPitchSign = 1;                 // Pitch axis sign correction / تصحيح إشارة محور أمام/خلف / 俯仰轴符号修正
constexpr int kYawSign = 1;                  // Yaw axis sign correction / تصحيح إشارة محور الانعراج / 偏航轴符号修正

// Motor Weight and Trim Settings
// إعدادات وزن وترميم المحركات
// 中文: 电机重量和微调设置
constexpr float kMotorWeightMin = 0.85f;         // Minimum motor weight factor / أقل عامل وزن المحرك / 最小电机重量系数
constexpr float kMotorWeightMax = 1.15f;         // Maximum motor weight factor / أكبر عامل وزن المحرك / 最大电机重量系数
constexpr int kMotorTrimMinUs = -120;            // Minimum trim adjustment / أقل تعديل ترميم / 最小微调量
constexpr int kMotorTrimMaxUs = 120;             // Maximum trim adjustment / أكبر تعديل ترميم / 最大微调量
constexpr float kWeightOptimizeGain = 0.0030f;   // Motor weight optimization gain / ربح تحسين وزن المحرك / 电机重量优化增益

// PID Controller Enhanced Settings
// إعدادات متحكم PID المحسّنة (لفلترة المشتقات وتحديد المعدل)
// 中文: PID 控制器增强设置（用于导数滤波和变化率限制）
constexpr float kPidDerivativeFilterAlpha = 0.28f;         // 0-1: derivative smoothing (0=smooth, 1=responsive)
constexpr float kPidRateLimiterMaxChangePerSec = 26000.0f;  // Maximum motor output change per second (usec)

// Default Wi-Fi Settings
// إعدادات Wi-Fi الافتراضية
// 中文: 默认 Wi-Fi 设置
constexpr uint8_t kWifiApChannel = 6;                      // AP channel / قناة نقطة الوصول / AP 信道
constexpr uint8_t kWifiApMaxConnections = 4;              // AP max clients / أقصى عدد عملاء / AP 最大连接数
constexpr char kDefaultApSsid[] = "Drone-S3-CAM";           // Default AP SSID / اسم AP افتراضي / 默认 AP SSID
constexpr char kDefaultApPassword[] = "12345678";            // Default AP password / كلمة مرور AP افتراضية / 默认 AP 密码

constexpr char kDefaultStaSsid[] = "honor";               // Default STA SSID / اسم STA افتراضي / 默认 STA SSID
constexpr char kDefaultStaPassword[] = "2000320003";        // Default STA password / كلمة مرور STA افتراضية / 默认 STA 密码
constexpr bool kDefaultStaEnabled = true;                    // Enable STA by default / تفعيل STA افتراضياً / 默认启用 STA

}  // namespace DroneConfig
