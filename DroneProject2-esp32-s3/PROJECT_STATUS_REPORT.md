# تقرير الحالة النهائية للمشروع

## 1. تعريف المشروع

هذا المشروع هو متحكم طيران مخصص لطائرة رباعية يعتمد على ESP32-S3 ويستخدم BNO055 كمصدر بيانات IMU، مع واجهة Wi-Fi ولوحة تحكم ويب وشاشة TFT.

هدف المشروع الحالي هو:

- تشغيل طائرة رباعية X-Quad.
- قراءة أوامر الريموت عبر SBUS.
- تثبيت الطائرة في وضع `STAB`.
- إتاحة عرض الحالة والاتصال عبر الويب.
- الحفاظ على توافق التوصيلات الحالية للوحة والأسلاك.

## 2. العتاد المستخدم

- المعالج: `ESP32-S3`
- IMU: `Bosch BNO055`
- بروتوكول الريموت: `SBUS`
- المحركات: `2212 KV920`
- الـ ESC: `Hobbywing Skywalker 40A V2`
- البطارية: `3S-4S LiPo`
- الإطار: `450-550`

## 3. البنية البرمجية الحالية

الكود الفعال موجود داخل:

- [src/drone/Espfc/src/App](c:/Users/darkh/Desktop/PROGCTS/esp32s3_cam/src/drone/Espfc/src/App)

أهم الوحدات:

- `MAIN`: نقطة البداية الرئيسية.
- `INPUT`: الإعدادات، الأنواع، التخزين الدائم.
- `AHRS`: قراءة الـ IMU وتحويلها إلى Roll / Pitch / Yaw.
- `Control`: منطق الطيران وحلقة التحكم.
- `ESC`: خلط المحركات وإخراج PWM.
- `RC`: قراءة SBUS.
- `OUTPUT`: الشاشة وواجهة السيريال.
- `WIFI`: الشبكة ولوحة الويب.
- `SENSOR`: GPS.

## 4. تعيين القنوات والمحركات النهائي

تم تثبيت الترتيب النهائي كما يلي:

### المحركات

- `M1 = Front-Left`
- `M2 = Front-Right`
- `M3 = Rear-Left`
- `M4 = Rear-Right`

القيم الفعلية موجودة في:

- [src/drone/Espfc/src/App/INPUT/drone_config.h](c:/Users/darkh/Desktop/PROGCTS/esp32s3_cam/src/drone/Espfc/src/App/INPUT/drone_config.h)

وترتيب الأرجل الحالي:

- `M1 -> GPIO38`
- `M2 -> GPIO39`
- `M3 -> GPIO40`
- `M4 -> GPIO37`

### قنوات الريموت

- `CH1 = Roll`
- `CH2 = Pitch`
- `CH3 = Throttle`
- `CH4 = Yaw`
- `CH5 = Arm`
- `CH6 = Stabilize/Sensor`
- `CH7 = Aux1`
- `CH8 = Web Link`

### اتجاهات القنوات المطلوبة والمطبقة

- `CH1`: `2000 = يمين` ، `1000 = يسار`
- `CH2`: `2000 = قدام` ، `1000 = ورا`
- `CH3`: `1000..2000 = Throttle`
- `CH4`: `2000 = Yaw Right` ، `1000 = Yaw Left`

الاتجاهات البرمجية الحالية:

- `kRcRollDirection = 1`
- `kRcPitchDirection = -1`
- `kRcYawDirection = 1`

## 5. أهم المشاكل التي كانت موجودة

خلال العمل على المشروع تم تشخيص ومعالجة عدة مشاكل متراكبة، أهمها:

1. عدم تطابق بين ترتيب المحركات الفعلي وبين الخلط البرمجي.
2. عدم اتساق بعض نصوص واجهة الويب مع تعيين القنوات الفعلي.
3. خلط بين اتجاهات الريموت واتجاهات التغذية الراجعة الخاصة بالتوازن.
4. تعيين غير صحيح لمحاور الـ IMU أدى إلى استجابة خاطئة في Roll وPitch.
5. انعكاس في بعض استجابات التوازن يمين/يسار وأمام/خلف.
6. بطء محسوس في الاستجابة بسبب أعمال خلفية داخل المسار الحرج لحلقة التحكم.
7. بطء إضافي في استجابة المحركات بسبب `slew limiting` شديد جداً.

## 6. ما الذي تم تعديله فعلياً

### 6.1 تصحيح تعيين المحركات والقنوات

تم تثبيت ترتيب المحركات والقنوات في:

- [src/drone/Espfc/src/App/INPUT/drone_config.h](c:/Users/darkh/Desktop/PROGCTS/esp32s3_cam/src/drone/Espfc/src/App/INPUT/drone_config.h)

### 6.2 تصحيح خلط المحركات

تم تعديل إشارات المزج في:

- [src/drone/Espfc/src/App/ESC/motor_mixer.cpp](c:/Users/darkh/Desktop/PROGCTS/esp32s3_cam/src/drone/Espfc/src/App/ESC/motor_mixer.cpp)

الترتيب الحالي خاص بـ X-Quad مع:

- `Pitch mix signs = {1, 1, -1, -1}`
- `Roll mix signs = {1, -1, 1, -1}`
- `Yaw mix signs = {1, -1, -1, 1}`

### 6.3 فصل اتجاه الريموت عن اتجاه التوازن

كان منطق التوازن يستخدم اتجاهات `RC` نفسها أثناء تفسير بيانات الـ IMU، وهذا سبب أخطاء في الاستقرار.

تم تصحيح هذا في:

- [src/drone/Espfc/src/App/Control/flight_controller.cpp](c:/Users/darkh/Desktop/PROGCTS/esp32s3_cam/src/drone/Espfc/src/App/Control/flight_controller.cpp)

الوضع الحالي:

- الريموت له اتجاهاته الخاصة.
- التوازن يقرأ الـ IMU مباشرة بدون إعادة استخدام اتجاهات الريموت.

### 6.4 تصحيح محاور الـ BNO055

تم تصحيح تفسير محاور الزوايا والسرعات في:

- [src/drone/Espfc/src/App/AHRS/ahrs_manager.cpp](c:/Users/darkh/Desktop/PROGCTS/esp32s3_cam/src/drone/Espfc/src/App/AHRS/ahrs_manager.cpp)

الوضع الحالي:

- `yaw <- euler.x`
- `roll <- euler.z`
- `pitch <- euler.y`
- `roll_rate <- gyro.y`
- `pitch_rate <- gyro.x`
- `yaw_rate <- gyro.z`

كما تم تعديل التقاط `level offsets` ليتوافق مع نفس التفسير.

### 6.5 تصحيح إشارات التوازن

الإشارات الحالية في النظام:

- `kRollSign = -1`
- `kPitchSign = 1`
- `kYawSign = 1`

وهذا يمثل آخر وضع استقرت عليه التعديلات الخاصة بمنظومة التوازن.

### 6.6 تحسين عرض المعلومات والتشخيص

تمت إضافة رسائل إقلاع واضحة في السيريال داخل:

- [src/drone/Espfc/src/App/OUTPUT/serial_console.cpp](c:/Users/darkh/Desktop/PROGCTS/esp32s3_cam/src/drone/Espfc/src/App/OUTPUT/serial_console.cpp)

الرسائل تعرض:

- خريطة قنوات الريموت.
- خريطة المحركات.
- حالة الدرايفرات الأساسية.

### 6.7 تحديث واجهة الويب لتتوافق مع الواقع

تم تعديل نصوص الخريطة والعناوين داخل واجهة الويب لتتوافق مع:

- `CH1 Roll`
- `CH2 Pitch`
- `CH3 Throttle`
- `CH4 Yaw`
- `CH5 Arm`
- `CH6 Stabilize`
- `CH7 Aux1`
- `CH8 Web Link`

## 7. خصائص النظام الحالية

### إعدادات التحكم والزمنيات

القيم الحالية في الكود:

- `Control Loop Period = 4000us`
- `Control Loop Target = ~250Hz`
- `IMU Read Period = 5000us`
- `IMU Read Target = ~200Hz`
- `GPS Refresh Period = 25000us`
- `GPS Target = ~40Hz`
- `TFT Refresh = 500ms`
- `TFT Refresh While Armed = 1500ms`
- `Internal Web Service Period = 20ms`

هذه القيم موجودة في:

- [src/drone/Espfc/src/App/INPUT/drone_config.h](c:/Users/darkh/Desktop/PROGCTS/esp32s3_cam/src/drone/Espfc/src/App/INPUT/drone_config.h)

### خصائص خرج المحركات

- `ESC range = 1000..2000us`
- `PWM frequency = 200Hz`
- `PWM resolution = 14-bit`
- `Motor slew limit = 120us per update`

### خصائص الريموت

- `SBUS baud = 100000`
- `SBUS inverted = true`
- `Failsafe timeout = 300000us`
- `Deadband = 25us`

### خصائص الـ IMU

- يعمل عبر `I2C` على `400kHz`
- يحاول العنوان `0x28` ثم `0x29`
- `External crystal = disabled`
- وضع التشغيل الحالي: `NDOF`

## 8. الإعدادات الافتراضية الحالية في الكود

الإعدادات الافتراضية المخزنة برمجياً موجودة في:

- [src/drone/Espfc/src/App/INPUT/settings_store.cpp](c:/Users/darkh/Desktop/PROGCTS/esp32s3_cam/src/drone/Espfc/src/App/INPUT/settings_store.cpp)

القيم الافتراضية الحالية:

- `Roll PID = 2.5 / 0.03 / 12.0`
- `Pitch PID = 2.5 / 0.03 / 12.0`
- `Yaw PID = 3.0 / 0.02 / 0.0`
- `Max Angle = 26`
- `RC Expo = 0.22`
- `Manual Mix = 190`
- `Yaw Mix = 130`
- `Motor Idle = 1040`

ملاحظة مهمة:

هذه قيم `Defaults` داخل الكود، لكن القيم الفعلية على اللوحة قد تختلف إذا كانت هناك إعدادات محفوظة سابقاً في `Preferences`.

## 9. لماذا كانت الاستجابة بطيئة

بعد التحليل، تبين أن البطء لم يكن من سبب واحد فقط، بل من عدة عوامل:

1. وجود بعض أعمال الخلفية مثل الويب وGPS والشاشة داخل دورة العمل العامة.
2. استخدام BNO055 في وضع fused orientation، وهذا الحساس ليس الأسرع في تطبيقات الطيران العنيف.
3. وجود `motor slew limit` صغير جداً سابقاً، ما جعل خرج المحركات يتدرج ببطء.
4. وجود `RC Expo` افتراضي عند `0.22`، وهذا يعطي شعوراً بأن منتصف العصا ناعم أو بطيء.
5. `Max Angle = 26` يجعل وضع `STAB` محافظاً أكثر من كونه حاد الاستجابة.

## 10. ما الذي تم عمله لتحسين الاستجابة

تم تنفيذ التحسينات التالية:

1. نقل أعمال الخلفية بعيداً عن المسار الحرج لحلقة التحكم قدر الإمكان.
2. تقليل تحديث شاشة الـ TFT أثناء التسليح لتقليل الحمل.
3. رفع هدف حلقة التحكم من `200Hz` تقريباً إلى `250Hz` تقريباً.
4. رفع هدف قراءة الـ IMU من `125Hz` تقريباً إلى `200Hz` تقريباً.
5. رفع `MotorOutputSlewUs` من قيمة صغيرة جداً إلى `120us` لتقليل بطء استجابة الموتورات.

## 11. الحالة الحالية لوضع التثبيت

الوضع الحالي يمثل آخر نقطة وصلنا لها في تصحيح التوازن:

- ترتيب المحركات ثابت.
- ترتيب القنوات ثابت.
- محاور الـ IMU أعيد تعيينها لتتوافق مع جسم الطائرة.
- إشارات التوازن تم تعديلها لتطابق اتجاه الاستجابة المطلوب.

لكن يظل هناك تنبيه مهم:

- هذا النظام ما زال يعتمد على `BNO055 fused orientation`، لذلك لن يعطي نفس سرعة واستجابة متحكمات الطيران الاحترافية التي تعتمد على Gyro raw سريع مع مرشح مخصص.

## 12. التوصيات الفنية الحالية

إذا كان الهدف طيراناً مستقراً واختبارات تدريجية:

- `Loop target الحالي` مناسب كبداية.
- `Max Angle` يمكن رفعه لاحقاً إلى `30-35` إذا لزم.
- `RC Expo` يمكن خفضه إلى `0.10-0.15` إذا بقي الإحساس بطيئاً حول المنتصف.

إذا كان الهدف استجابة أسرع جداً أو طيران أكثر حدة:

- الترقية الحقيقية ستكون بالانتقال إلى قراءة Gyro raw مباشرة وبناء `rate loop` داخلي سريع.
- BNO055 مناسب للـ leveling والبدايات، لكنه ليس أفضل خيار لـ acro أو استجابة عدوانية.

## 13. آخر تعديل تم عمله

آخر تعديل تم عمله كان متعلقاً بتقليل كمون الاستجابة، وشمل:

- رفع `kControlPeriodUs` إلى `4000us`
- رفع `kImuReadPeriodUs` إلى `5000us`
- رفع `kMotorOutputSlewUs` إلى `120us`

هذا التعديل موجود في:

- [src/drone/Espfc/src/App/INPUT/drone_config.h](c:/Users/darkh/Desktop/PROGCTS/esp32s3_cam/src/drone/Espfc/src/App/INPUT/drone_config.h)

## 14. حالة الاختبار الحالية

الحالة الحالية لهذه التعديلات هي:

- تم بناء المشروع بنجاح بعد آخر تعديل.
- آخر تعديل الخاص بالاستجابة السريعة **لم يتم اختباره عملياً على الطائرة حتى الآن**.
- لم يتم في آخر خطوة تأكيد تجربة طيران فعلية أو اختبار حي على العتاد بعد هذا التعديل الأخير.

بمعنى أوضح:

- آخر نسخة في الكود: محدثة.
- آخر نسخة مبنية: ناجحة.
- آخر نسخة مجربة ميدانياً: ليست مؤكدة بعد بالنسبة لآخر تعديل على السرعة.

## 15. وضع المشروع الحالي باختصار

المشروع حالياً في مرحلة:

- استقرار تعيينات القنوات والمحركات.
- استقرار التوصيلات الأساسية.
- تصحيح معظم أخطاء التوازن والمحاور.
- تحسين مسار الاستجابة وتقليل البطء.
- بقاء خطوة مهمة أخيرة: `اختبار ميداني بعد آخر تعديل`.

## 16. ملفات مهمة للمراجعة السريعة

- [platformio.ini](c:/Users/darkh/Desktop/PROGCTS/esp32s3_cam/platformio.ini)
- [src/drone/Espfc/src/App/ARCHITECTURE.md](c:/Users/darkh/Desktop/PROGCTS/esp32s3_cam/src/drone/Espfc/src/App/ARCHITECTURE.md)
- [src/drone/Espfc/src/App/INPUT/drone_config.h](c:/Users/darkh/Desktop/PROGCTS/esp32s3_cam/src/drone/Espfc/src/App/INPUT/drone_config.h)
- [src/drone/Espfc/src/App/INPUT/settings_store.cpp](c:/Users/darkh/Desktop/PROGCTS/esp32s3_cam/src/drone/Espfc/src/App/INPUT/settings_store.cpp)
- [src/drone/Espfc/src/App/AHRS/ahrs_manager.cpp](c:/Users/darkh/Desktop/PROGCTS/esp32s3_cam/src/drone/Espfc/src/App/AHRS/ahrs_manager.cpp)
- [src/drone/Espfc/src/App/Control/flight_controller.cpp](c:/Users/darkh/Desktop/PROGCTS/esp32s3_cam/src/drone/Espfc/src/App/Control/flight_controller.cpp)
- [src/drone/Espfc/src/App/ESC/motor_mixer.cpp](c:/Users/darkh/Desktop/PROGCTS/esp32s3_cam/src/drone/Espfc/src/App/ESC/motor_mixer.cpp)
- [src/drone/Espfc/src/App/OUTPUT/serial_console.cpp](c:/Users/darkh/Desktop/PROGCTS/esp32s3_cam/src/drone/Espfc/src/App/OUTPUT/serial_console.cpp)

## 17. ملاحظة ختامية

هذا التقرير يمثل آخر حالة موثقة للمشروع داخل بيئة التطوير الحالية، وهو مناسب كمستند مرجعي إذا احتجت:

- شرح المشروع لشخص آخر.
- توثيق حالة العمل الحالية.
- تسليم البيانات التقنية للمشروع.
- الرجوع لاحقاً إلى ما تم تعديله ولماذا.

## 18. تدفق النظام الكامل وقت التشغيل

تدفق النظام الحالي يعمل بالشكل التالي:

1. يبدأ التنفيذ من `main.cpp` ثم يستدعي `FlightController::setup()`.
2. في `setup()` يتم تحميل الإعدادات من `Preferences` ثم تهيئة الوحدات التالية بالترتيب:
	- السيريال
	- مستقبل SBUS
	- خرج المحركات
	- IMU
	- GPS
	- واجهة الويب
	- شاشة TFT
3. بعد الإقلاع، الدالة `loop()` تستدعي `FlightController::loop()` باستمرار.
4. داخل `FlightController::loop()` يتم:
	- تحديث الـ IMU
	- التحقق من مرور زمن دورة التحكم
	- تنفيذ الأعمال الخلفية إذا لم يحن وقت دورة التحكم بعد
	- أخذ snapshot من `RC` و`IMU` و`GPS`
	- تحديد وضع الويب من `CH8`
	- تحويل قنوات الريموت إلى أوامر تحكم فعلية
	- تحديد حالة التسليح `armed / disarmed`
	- بناء بيانات التليمترية
	- حساب أوامر PID أو التحكم اليدوي
	- خلط القيم إلى 4 محركات
	- كتابة PWM للمحركات
	- تخزين التليمترية للاستخدام في الويب والشاشة

## 19. مسؤوليات كل وحدة برمجية

### `MAIN`

- فقط نقطة دخول وتشغيل `FlightController`.

### `Control / flight_controller`

- القلب الرئيسي للنظام.
- ينسق بين جميع الوحدات.
- يقرر وضع `MANUAL` أو `STAB`.
- يحسب `roll / pitch / yaw` setpoints.
- يطبق PID ثم يرسل الناتج إلى المزج.

### `AHRS / ahrs_manager`

- مسؤول عن تهيئة BNO055 وقراءة `VECTOR_EULER` و`VECTOR_GYROSCOPE`.
- يجهز `ImuSnapshot` الجاهز للاستخدام.
- يطبق `roll/pitch offsets`.
- يلتقط `level trim` عند طلب المعايرة.

### `RC / rc_receiver`

- يقرأ SBUS عبر UART2.
- يفك شيفرة 8 قنوات.
- يحول القيم إلى `1000..2000us`.
- يحدد freshness لكل قناة وحالة `frame_fresh`.

### `ESC / motor_mixer`

- يطبق خلط X-Quad.
- يجمع `throttle + pitch + roll + yaw`.
- يطبق `motor_weight` و`motor_trim`.
- يطبق `slew limiting` على خرج المحركات.

### `ESC / motor_output`

- يهيئ LEDC على ESP32-S3.
- يحول `pulse_us` إلى duty cycle.
- يكتب القيم إلى 4 قنوات PWM.

### `INPUT / settings_store`

- يحمل الإعدادات من `Preferences`.
- يجهز الإعدادات الافتراضية.
- ينظف القيم ويحصرها في حدود آمنة.
- يدعم ترحيل الإصدارات القديمة `V3 / V4`.

### `WIFI / web_ui`

- يوفر واجهات HTTP.
- يعرض الحالة السريعة والثقيلة.
- يقرأ ويكتب الإعدادات.
- يسمح بمعايرة المستوى وتحديث GPS home.

### `WIFI / wifi_manager`

- يدير AP وSTA.
- يحافظ على استقرار AP عند وجود عميل متصل.
- يعيد محاولة الاتصال بـ STA عند الحاجة.

### `Connect / web_link`

- يحدد وضع الويب من `CH8`.
- يمنع استقبال الأوامر في وضع `broadcast-only`.
- يعطي تلميحات polling/update حسب الوضع.

### `OUTPUT / tft_status`

- يعرض الحالة المختصرة على الشاشة.
- يعرض RC / IMU / GPS / Wi-Fi.
- تم تخفيض تردده أثناء التسليح لتقليل الحمل.

### `SENSOR / gps_manager`

- يقرأ GPS عبر UART.
- يحلل NMEA بواسطة `TinyGPSPlus`.
- يدير home point وحساب distance / bearing.

## 20. واجهات الويب والـ API الحالية

المسارات الأساسية الموجودة في `web_ui.cpp`:

- `/` : الصفحة الرئيسية.
- `/api/state` : حالة سريعة.
- `/api/state-heavy` : حالة تفصيلية.
- `/api/settings` : قراءة وكتابة الإعدادات.
- `/api/pid` : تحديث PID.
- `/api/calibrate-level` : معايرة المستوى.
- `/api/home/set` : تثبيت موقع home من GPS.
- `/api/home/clear` : مسح home.

ملاحظات تشغيلية:

- بعض الأوامر تُحجب إذا كان `CH8` في وضع يمنع استقبال الأوامر.
- النظام يرسل `telemetry snapshots` إلى الويب بدلاً من القراءة المباشرة من الوحدات في كل endpoint.

## 21. نتائج المراجعة البرمجية

بعد مراجعة الوحدات الأساسية، هذه أهم النتائج الحالية:

1. تصميم النظام منظم ومقسّم إلى وحدات واضحة، وهذا يسهل الصيانة والتطوير.
2. تدفق البيانات من `RC/IMU/GPS` إلى `FlightController` ثم إلى `MotorMixer/MotorOutput` واضح ومباشر.
3. استخدام `TelemetrySnapshot` و`FlightSettings` مناسب جداً كواجهة مشتركة بين الوحدات.
4. تم تقليل الحمل في المسار الحرج مقارنة بالحالة السابقة، لكن النظام ما زال يعتمد على BNO055 fused data، وهذا يضع سقفاً أعلى للاستجابة.
5. النظام الحالي أقرب إلى `angle stabilize controller` منه إلى متحكم طيران احترافي يعتمد `rate loop` داخلي سريع.

### ملاحظات ومخاطر قائمة

1. يوجد سلوك مهم في [src/drone/Espfc/src/App/INPUT/settings_store.cpp](c:/Users/darkh/Desktop/PROGCTS/esp32s3_cam/src/drone/Espfc/src/App/INPUT/settings_store.cpp): منطق `Sanitize()` يعيد تفعيل `STA` تلقائياً ويملأ بياناته الافتراضية إذا كانت `sta_enabled = false` أو كانت بيانات STA فارغة. هذا يعني أن تعطيل STA من الإعدادات قد لا يبقى معطلاً كما يتوقع المستخدم.
2. في [src/drone/Espfc/src/App/AHRS/ahrs_manager.cpp](c:/Users/darkh/Desktop/PROGCTS/esp32s3_cam/src/drone/Espfc/src/App/AHRS/ahrs_manager.cpp) هناك انتظار إقلاع حتى `7000ms` لمحاولة الوصول إلى calibration مقبول. هذا ليس خطأً في الطيران، لكنه يضيف تأخير إقلاع ملحوظ.
3. الاعتماد على `VECTOR_EULER` من BNO055 يجعل استجابة `STAB` محدودة مقارنة باستخدام gyro raw + complementary/kalman/filter مخصص.
4. لا يوجد حالياً battery sensing فعلي رغم وجود حقول telemetry لهذا الغرض.
5. النظام لا يحتوي Blackbox أو logging داخلي لتحليل الرحلات أو الاهتزازات.

## 22. تقييم النظام الحالي ككل

التقييم الحالي للنظام:

- كنظام تجريبي مخصص: جيد ومنظم وقابل للتطوير.
- كنظام للتثبيت الأساسي والطيران الحذر: مناسب بعد التعديلات الأخيرة.
- كنظام لاستجابة حادة جداً أو طيران أكروباتي: ما زال محدوداً بسبب نوع IMU وطبيعة حلقة التحكم الحالية.

الوضع النهائي الذي وصلنا له الآن:

- البنية العامة مستقرة.
- القنوات والمحركات والمحاور موثقة ومصححة.
- الويب، الشاشة، السيريال، وGPS مدمجة مع التليمترية.
- آخر تعديل لتقليل الكمون موجود في الكود.
- آخر تعديل ما زال بحاجة اختبار ميداني نهائي للتأكد من أثره الفعلي على الطائرة.
