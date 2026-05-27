# ESP32-S3 Drone App Layout

English: This `App` tree is the active drone firmware. It is kept separate from the embedded Espfc sources so the build stays clean and easier to extend.

العربية: مجلد `App` هو مسار الفيرموير الفعّال للطائرة. تم فصله عن ملفات Espfc المدمجة حتى يبقى البناء أنظف وأسهل للتطوير.

中文: `App` 目录是当前无人机固件的活动代码。它与内嵌的 Espfc 源码分离，便于构建、维护和扩展。

---

## 📁 System Components / مكونات النظام / 系统组件

### `MAIN` - Entry Point / نقطة الدخول / 入口点
**English**: Contains the main Arduino setup() and loop() functions that start the flight controller.
- `main.cpp`: Initializes and runs the FlightController

**العربية**: يحتوي على دوال Arduino الرئيسية setup() و loop() التي تشغل FlightController.
- `main.cpp`: يقوم بتشغيل وإدارة FlightController

**中文**: 包含主要的 Arduino setup() 和 loop() 函数，用于启动飞控。
- `main.cpp`: 初始化并运行 FlightController

---

### `INPUT` - Configuration & Settings / الإعدادات والبيانات / 配置与设置
**English**: Handles all input data, configuration, and persistent settings.
- `drone_config.h`: Hardware pins and system constants
- `drone_types.h`: Data structures for the entire system
- `settings_store.*`: Save/load settings to flash memory

**العربية**: يتعامل مع كل بيانات الإدخال، الإعدادات، والبيانات المحفوظة.
- `drone_config.h`: دبابيس الأجهزة وثوابت النظام
- `drone_types.h`: هياكل البيانات للنظام بأكمله
- `settings_store.*`: حفظ/تحميل الإعدادات في ذاكرة الفلاش

**中文**: 处理所有输入数据、配置和持久化设置。
- `drone_config.h`: 硬件引脚和系统常量
- `drone_types.h`: 整个系统的数据结构
- `settings_store.*`: 保存/加载设置到闪存

---

### `AHRS` - Attitude & Heading / الاتجاه والتوجيه / 姿态与航向
**English**: Calculates drone orientation using IMU sensors.
- Reads accelerometer, gyroscope, magnetometer
- Provides roll, pitch, yaw angles
- Handles sensor calibration

**العربية**: يحسب اتجاه الطائرة باستخدام مستشعرات IMU.
- يقرأ التسارع، الجيروسكوب، المغناطيس
- يوفر زوايا roll, pitch, yaw
- يتعامل مع معايرة المستشعرات

**中文**: 使用 IMU 传感器计算无人机姿态。
- 读取加速度计、陀螺仪、磁力计
- 提供横滚、俯仰、偏航角
- 处理传感器校准

---

### `Control` - Flight Control / التحكم في الطيران / 飞行控制
**English**: Main flight control logic and mathematics.
- `flight_controller.*`: Core flight loop
- PID controllers for stable flight
- Flight mode management (manual/stabilize)

**العربية**: منطق التحكم الرئيسي في الطيران والرياضيات.
- `flight_controller.*`: حلقة الطيران الأساسية
- متحكمات PID للطيران المستقر
- إدارة أوضاع الطيران (يدوي/توازن)

**中文**: 主要飞控逻辑和数学运算。
- `flight_controller.*`: 核心飞控循环
- 稳定飞行的 PID 控制器
- 飞行模式管理（手动/稳定）

---

### `ESC` - Motor Control / تحكم المحركات / 电机控制
**English**: Controls brushless motors and mixing.
- Motor PWM signal generation
- Mixer calculations for 4 motors
- Motor safety and arm/disarm logic

**العربية**: يتحكم في المحركات بدون فرش والمكسر.
- توليد إشارات PWM للمحركات
- حسابات المكسر لـ 4 محركات
- منطق الأمان والتسليح/إلغاء التسليح

**中文**: 控制无刷电机和混控。
- 电机 PWM 信号生成
- 4 电机的混控计算
- 电机安全和上锁/解锁逻辑

---

### `OUTPUT` - Display & Communication / الشاشة والاتصال / 显示与通信
**English**: Handles visual output and data display.
- `tft_status.*`: TFT screen display
- Serial monitor output
- LED status indicators

**العربية**: يتعامل مع المخرجات البصرية وعرض البيانات.
- `tft_status.*`: شاشة TFT
- مخرجات الشاشة التسلسلية
- مؤشرات حالة LED

**中文**: 处理视觉输出和数据显示。
- `tft_status.*`: TFT 屏幕显示
- 串口监视器输出
- LED 状态指示器

---

### `RC` - Radio Control / التحكم عن بعد / 无线控制
**English**: Processes receiver input signals.
- SBUS signal reading from a single UART input
- Channel mapping and calibration
- Failsafe detection

**العربية**: يعالج إشارات المستقبل.
- قراءة إشارة SBUS من دخل UART واحد
- تخطيط ومعايرة القنوات
- كشف فقدان الإشارة

**中文**: 处理接收机输入信号。
- 通过单个 UART 输入读取 SBUS 信号
- 通道映射和校准
- 失控检测

---

### `SENSOR` - Sensors & GPS / المستشعرات والـ GPS / 传感器与GPS
**English**: External sensor integration.
- GPS module communication
- Position and altitude tracking
- Future sensor expansion

**العربية**: تكامل المستشعرات الخارجية.
- تواصل مع وحدة GPS
- تتبع الموقع والارتفاع
- توسعة المستشعرات المستقبلية

**中文**: 外部传感器集成。
- GPS 模块通信
- 位置和高度跟踪
- 未来传感器扩展

---

### `WIFI` - Network & Web Interface / الشبكة وواجهة الويب / 网络与网页界面
**English**: Wi-Fi connectivity and web dashboard.
- Access Point mode (AP)
- Station mode (STA) for router connection
- Multi-language web interface
- Real-time telemetry display

**العربية**: اتصال Wi-Fi ولوحة تحكم الويب.
- وضع نقطة الوصول (AP)
- وضع المحطة (STA) للاتصال بالراوتر
- واجهة ويب متعددة اللغات
- عرض التليمترية المباشر

**中文**: Wi-Fi 连接和网页仪表板。
- 接入点模式 (AP)
- 站点模式 (STA) 连接路由器
- 多语言网页界面
- 实时遥测显示

---

### `Connect` - Communication Logic / منطق الاتصال / 通信逻辑
**English**: Manages communication modes and data flow.
- CH8 link mode control
- Data priority management
- Connection state handling

**العربية**: يدير أوضاع الاتصال وتدفق البيانات.
- التحكم في وضع رابط CH8
- إدارة أولوية البيانات
- التعامل مع حالة الاتصال

**中文**: 管理通信模式和数据流。
- CH8 链路模式控制
- 数据优先级管理
- 连接状态处理

---

## ⚠️ Safety Notes / ملاحظات الأمان / 安全注意事项

### English:
- Never connect the camera ribbon in this build
- Never insert a MicroSD card while ESC lines use GPIO37..40
- Power the board from one ESC BEC only
- Keep PSRAM disabled for this wiring profile

### العربية:
- لا تصل شريط الكاميرا في هذا البناء
- لا تدخل بطاقة MicroSD بينما تستخدم خطوط ESC GPIO37..40
- قم بتغذية اللوحة من ESC BEC واحد فقط
- حافظ على تعطيل PSRAM لهذا التوصيل

### 中文:
- 此构建中切勿连接摄像头排线
- ESC 线路使用 GPIO37..40 时切勿插入 MicroSD 卡
- 仅使用一个 ESC BEC 为电路板供电
- 此接线配置请保持 PSRAM 禁用

---

## 🎮 CH8 Control Modes / أوضاع تحكم CH8 / CH8控制模式

### English:
- **1000µs**: AP mode + commands enabled (broadcasts Wi-Fi, accepts web commands)
- **1500µs**: Read-only mode (displays data, blocks commands)
- **2000µs**: STA mode + commands enabled (connects to router, accepts web commands)

### العربية:
- **1000µs**: وضع AP + الأوامر مفعلة (يبث Wi-Fi، يقبل أوامر الويب)
- **1500µs**: وضع القراءة فقط (يعرض البيانات، يمنع الأوامر)
- **2000µs**: وضع STA + الأوامر مفعلة (يتصل بالراوتر، يقبل أوامر الويب)

### 中文:
- **1000µs**: AP 模式 + 命令启用（广播 Wi-Fi，接受网页命令）
- **1500µs**: 只读模式（显示数据，阻止命令）
- **2000µs**: STA 模式 + 命令启用（连接路由器，接受网页命令）

---

## 🌐 Default Network Settings / إعدادات الشبكة الافتراضية / 默认网络设置

### English:
- **AP Mode**: `Drone-S3-CAM` / `12345678`
- **STA Mode**: `honor` / `2000320003` (auto-enabled on first boot)
- **Languages**: Arabic, Chinese, English (saved in browser)

### العربية:
- **وضع AP**: `Drone-S3-CAM` / `12345678`
- **وضع STA**: `honor` / `2000320003` (يُفعل تلقائياً عند التشغيل الأول)
- **اللغات**: عربي، صيني، إنجليزي (تحفظ في المتصفح)

### 中文:
- **AP 模式**: `Drone-S3-CAM` / `12345678`
- **STA 模式**: `honor` / `2000320003`（首次启动时自动启用）
- **语言**: 阿拉伯语、中文、英语（保存在浏览器中）
