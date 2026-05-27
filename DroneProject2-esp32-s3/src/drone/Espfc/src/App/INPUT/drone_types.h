#pragma once

#include <Arduino.h>

// PID Controller Structure
// هيكل متحكم PID
// 中文: PID 控制器结构
struct PidTuning {
  float kp;              // Proportional gain / ربح التناسب / 比例增益
  float ki;              // Integral gain / ربح التكامل / 积分增益
  float kd;              // Derivative gain / ربح المشتقة / 微分增益
  float integral_limit;    // Integral windup limit / حد تكامل الرياح / 积分饱和限制
  float output_limit;     // Output saturation limit / حد تشبع الخرج / 输出饱和限制
};

// Main Flight Settings Structure
// هيكل إعدادات الطيران الرئيسية
// 中文: 主要飞行设置结构
struct FlightSettings {
  uint32_t version;                    // Settings version / إصدار الإعدادات / 设置版本
  PidTuning roll_pid;                 // Roll axis PID / PID محور اليمين/يسار / 横滚轴 PID
  PidTuning pitch_pid;                // Pitch axis PID / PID محور أمام/خلف / 俯仰轴 PID
  PidTuning yaw_pid;                  // Yaw axis PID / PID محور الانعراج / 偏航轴 PID
  float max_angle_deg;                 // Maximum tilt angle / أقصى زاوية ميل / 最大倾斜角
  float rc_expo;                       // RC expo curve / منحنى expo للتحكم عن بعد / RC expo 曲线
  float manual_mix_us;                  // Manual mix rate / معدل الخلط اليدوي / 手动混控率
  float yaw_mix_us;                    // Yaw mix rate / معدل خلط الانعراج / 偏航混控率
  uint16_t motor_idle_us;               // Motor idle pulse / نبضة الخمول للمحرك / 电机怠速脉宽
  float motor_weight[4];                 // Motor weight factors / عوامل وزن المحركات / 电机重量系数
  int16_t motor_trim_us[4];              // Motor trim values / قيم ترميم المحركات / 电机微调值
  float level_roll_offset_deg;            // Roll level offset / إزاحة تسوية اليمين/يسار / 横滚水平偏移
  float level_pitch_offset_deg;           // Pitch level offset / إزاحة تسوية أمام/خلف / 俯仰水平偏移
  char ap_ssid[33];                     // Access Point SSID / اسم نقطة الوصول / 接入点 SSID
  char ap_password[65];                  // Access Point password / كلمة مرور نقطة الوصول / 接入点密码
  bool sta_enabled;                      // Station mode enabled / تفعيل وضع المحطة / 站点模式启用
  char sta_ssid[33];                     // Station SSID / اسم المحطة / 站点 SSID
  char sta_password[65];                  // Station password / كلمة مرور المحطة / 站点密码
};

// RC Receiver Snapshot
// لقطة مستقبل التحكم عن بعد
// 中文: 遥控接收机快照
struct RcSnapshot {
  uint16_t channels[8];    // RC channel values / قيم قنوات التحكم عن بعد / 遥控通道值
  bool fresh[8];            // Signal freshness / نضارة الإشارة / 信号新鲜度
  bool frame_fresh;          // Frame update flag / علم تحديث الإطار / 帧更新标志
};

// IMU Sensor Snapshot
// لقطة مستشعر IMU
// 中文: IMU 传感器快照
struct ImuSnapshot {
  bool ready;               // Sensor ready flag / علم جاهزية المستشعر / 传感器就绪标志
  bool calibrated;          // Sensor calibration status / حالة معايرة المستشعر / 传感器校准状态
  uint8_t cal_sys;          // System calibration / معايرة النظام / 系统校准
  uint8_t cal_gyro;         // Gyroscope calibration / معايرة الجيروسكوب / 陀螺仪校准
  uint8_t cal_accel;        // Accelerometer calibration / معايرة مقياس التسارع / 加速度计校准
  uint8_t cal_mag;          // Magnetometer calibration / معايرة المغناطيس / 磁力计校准
  float roll_deg;           // Roll angle / زاوية اليمين/يسار / 横滚角
  float pitch_deg;          // Pitch angle / زاوية أمام/خلف / 俯仰角
  float yaw_deg;            // Yaw angle / زاوية الانعراج / 偏航角
  float roll_rate_dps;      // Roll rate / معدل دوران اليمين/يسار / 横滚速率
  float pitch_rate_dps;     // Pitch rate / معدل دوران أمام/خلف / 俯仰速率
  float yaw_rate_dps;       // Yaw rate / معدل دوران الانعراج / 偏航速率
};

// GPS Sensor Snapshot
// لقطة مستشعر GPS
// 中文: GPS 传感器快照
struct GpsSnapshot {
  bool healthy;             // GPS health status / حالة صحة GPS / GPS 健康状态
  bool fix;                 // GPS fix status / حالة تثبيت GPS / GPS 定位状态
  bool home_set;            // Home point set / تعيين نقطة المنزل / 家点设置状态
  uint32_t satellites;       // Satellite count / عدد الأقمار الصناعية / 卫星数量
  uint32_t age_ms;           // GPS data age / عمر بيانات GPS / GPS 数据年龄
  float hdop;               // Horizontal dilution of precision / تخفيف الدقة الأفقية / 水平精度因子
  float speed_mps;          // Ground speed in m/s / سرعة الأرض بالمتر/ثانية / 地面速度（米/秒）
  float altitude_m;          // Altitude in meters / الارتفاع بالمتر / 高度（米）
  float course_deg;          // Course direction in degrees / الاتجاه بالدرجات / 航向角度
  double latitude;           // Latitude coordinate / خط العرض / 纬度坐标
  double longitude;          // Longitude coordinate / خط الطول / 经度坐标
  double home_latitude;      // Home latitude / خط العرض للمنزل / 家纬度
  double home_longitude;     // Home longitude / خط الطول للمنزل / 家经度
  float distance_home_m;      // Distance from home in meters / المسافة من المنزل بالمتر / 距家距离（米）
  float bearing_home_deg;     // Bearing to home in degrees / الاتجاه للمنزل بالدرجات / 到家方位角
};

// Main Telemetry Data Structure
// هيكل بيانات التليمترية الرئيسية
// 中文: 主要遥测数据结构
struct TelemetrySnapshot {
  uint32_t uptime_ms;                 // System uptime / تشغيل النظام / 系统运行时间
  uint32_t loop_hz;                   // Control loop frequency / تردد حلقة التحكم / 控制循环频率
  bool armed;                        // Arming status / حالة التسليح / 上锁状态
  bool stabilize_mode;                // Stabilize mode / وضع التثبيت / 稳定模式
  bool rc_ok;                         // RC signal status / حالة إشارة التحكم عن بعد / 遥控信号状态
  bool rc_failsafe;                   // RC failsafe status / حالة فقدان الإشارة / 失控状态
  bool imu_ok;                        // IMU status / حالة مستشعر IMU / IMU 状态
  bool imu_calibrated;                // IMU calibration ready / جاهزية معايرة IMU / IMU 校准就绪状态
  bool gps_ok;                        // GPS status / حالة مستشعر GPS / GPS 状态
  bool gps_fix;                       // GPS fix status / حالة تثبيت GPS / GPS 定位状态
  bool gps_home_set;                  // Home point set / تعيين نقطة المنزل / 家点设置状态
  uint8_t web_link_mode;               // Web link mode / وضع رابط الويب / 网页链接模式
  bool web_rx_allowed;                 // Web commands allowed / السماح بأوامر الويب / 网页命令允许
  uint16_t web_poll_hint_ms;           // Web polling hint / تلميح استقصاء الويب / 网页轮询提示
  uint16_t web_heavy_hint_ms;          // Web heavy update hint / تلميح التحديث الثقيل للويب / 网页重更新提示
  uint16_t web_link_ch8_us;            // CH8 PWM value / قيمة PWM لـ CH8 / CH8 PWM 值
  bool web_link_ch8_fresh;             // CH8 signal fresh / إشارة CH8 طازجة / CH8 信号新鲜度
  bool arm_switch_active;               // Arm switch state / حالة مفتاح التسليح / 上锁开关状态
  bool sensor_switch_active;             // Sensor switch state / حالة مفتاح المستشعرات / 传感器开关状态
  bool motor_output_ok;                // Motor output status / حالة خرج المحركات / 电机输出状态
  uint16_t rc_channels[8];              // Raw RC channel values / قيم RC الخام / 原始遥控通道值
  bool rc_fresh[8];                    // RC signal freshness / نضارة إشارة RC / 遥控信号新鲜度
  uint16_t motor_us[4];                 // Motor PWM values / قيم PWM للمحركات / 电机 PWM 值
  float throttle_percent;                // Throttle percentage / نسبة الخنق / 油门百分比
  float cpu_load_percent;                // Estimated control-loop load / حمل حلقة التحكم التقديري / 控制环路估算负载
  bool battery_available;               // Battery telemetry available / توفر قياس البطارية / 电池遥测是否可用
  float battery_voltage;                // Battery voltage / جهد البطارية / 电池电压
  float roll_deg;                      // Current roll angle / زاوية اليمين/يسار الحالية / 当前横滚角
  float pitch_deg;                     // Current pitch angle / زاوية أمام/خلف الحالية / 当前俯仰角
  float yaw_deg;                       // Current yaw angle / زاوية الانعراج الحالية / 当前偏航角
  float roll_setpoint_deg;              // Roll setpoint / نقطة ضبط اليمين/يسار / 横滚设定点
  float pitch_setpoint_deg;              // Pitch setpoint / نقطة ضبط أمام/خلف / 俯仰设定点
  float yaw_setpoint_dps;               // Yaw setpoint rate / نقطة ضبط سرعة الانعراج / 偏航速率设定点
  float roll_pid_output;                // Roll PID output / خرج PID لليمين/يسار / 横滚 PID 输出
  float pitch_pid_output;               // Pitch PID output / خرج PID لأمام/خلف / 俯仰 PID 输出
  float yaw_pid_output;                 // Yaw PID output / خرج PID للانعراج / 偏航 PID 输出
  uint8_t imu_cal_sys;                  // IMU system calibration / معايرة نظام IMU / IMU 系统校准
  uint8_t imu_cal_gyro;                 // IMU gyro calibration / معايرة جيروسكوب IMU / IMU 陀螺仪校准
  uint8_t imu_cal_accel;                // IMU accel calibration / معايرة تسارع IMU / IMU 加速度计校准
  uint8_t imu_cal_mag;                  // IMU mag calibration / معايرة مغناطيس IMU / IMU 磁力计校准
  uint32_t gps_satellites;               // GPS satellite count / عدد أقمار GPS / GPS 卫星数量
  float gps_hdop;                     // GPS precision / دقة GPS / GPS 精度
  float gps_speed_mps;                // GPS ground speed / سرعة GPS الأرضية / GPS 地面速度
  float gps_altitude_m;                // GPS altitude / ارتفاع GPS / GPS 高度
  float gps_course_deg;                // GPS course direction / اتجاه GPS / GPS 航向
  float gps_distance_home_m;            // GPS home distance / مسافة GPS للمنزل / GPS 到家距离
  float gps_bearing_home_deg;           // GPS home bearing / اتجاه GPS للمنزل / GPS 到家方位
  double gps_latitude;                 // GPS latitude / خط عرض GPS / GPS 纬度
  double gps_longitude;                // GPS longitude / خط طول GPS / GPS 经度
  double gps_home_latitude;             // GPS home latitude / خط عرض منزل GPS / GPS 家纬度
  double gps_home_longitude;            // GPS home longitude / خط طول منزل GPS / GPS 家经度
  char mode_name[16];                  // Flight mode name / اسم وضع الطيران / 飞行模式名称
  char arming_reason[32];              // Arming reason text / نص سبب التسليح / 上锁原因文本
  char ap_ssid[33];                    // Access Point SSID / اسم نقطة الوصول / 接入点 SSID
  char ap_password[65];                 // Access Point password / كلمة مرور نقطة الوصول / 接入点密码
  bool sta_enabled;                     // Station mode enabled / تفعيل وضع المحطة / 站点模式启用
  bool sta_connected;                   // Station connected status / حالة اتصال المحطة / 站点连接状态
  int32_t sta_rssi;                    // Station signal strength / قوة إشارة المحطة / 站点信号强度
  char sta_ssid[33];                    // Station SSID / اسم المحطة / 站点 SSID
  char ap_ip_address[16];               // Access Point IP / IP نقطة الوصول / 接入点 IP
  char sta_ip_address[16];              // Station IP / IP المحطة / 站点 IP
  char ip_address[16];                  // Current active IP / IP النشط الحالي / 当前活动 IP
};
