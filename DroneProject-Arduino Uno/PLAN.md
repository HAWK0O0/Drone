# خطة دمج GPS الجديد + تبسيط الويب + تحسين 3D

## Summary

ننفّذ دمج كامل لكود GPS الجديد داخل المشروع الحالي مع توافق كامل مع `FlightController`، ونحوّل واجهة الويب إلى نسخة أخف وأسلس مع 3D محسّن، دعم لغات `العربية/الإنجليزية/الصينية`، وتفعيل تعديل الإعدادات من الويب بشكل آمن (مع منع الحفظ أثناء `ARMED`). كما نحافظ على الاستفادة المستهدفة من مكتبات `Espfc` (GPS + PID) بشكل محافظ وآمن.

### Implementation Changes

- إنشاء/تحديث `gps_manager.h/.cpp` داخل `src/drone` اعتمادًا على ملفاتك في:
  `C:\Users\darkh\Desktop\PROGCTS\Drone project\DroneProject\include/src\gps_manager.*`
  مع تكييفها لواجهة هذا المشروع (`DroneConfig`, `FlightController`, `TelemetrySnapshot`).
- توحيد واجهة `GpsManager` لتدعم: `begin`, `update`, `snapshot`, `setHomeToCurrent`, `clearHome`, `setEnabled`, `isEnabled`.
- تطبيق سياسة GPS المطلوبة:
  GPS مفعّل عبر زر الريسيفر `SENS (CH7)` فقط، وإذا إشارة الريسيفر غير متاحة يصبح GPS غير قابل للاستخدام، مع الاحتفاظ بـ `Home` مخزنة.
- اعتماد fix gating محافظ: `>= 5` أقمار + بيانات حديثة (age محدود) + موقع صالح.
- ربط `FlightController` بسياسة تمكين GPS الجديدة قبل التحديث، وإضافة حقول Telemetry لازمة لحالة GPS التشغيلية/العمر الزمني.
- تحسين PID بشكل محافظ باستخدام صيغ مستعارة من `Espfc` (PT1-style filtering/معالجة آمنة لـ D-term) بدون إعادة كتابة عدوانية لمنطق التحكم.
- تفعيل الكتابة من الويب في `web_ui.cpp`:
  `POST /api/settings` (كل `FlightSettings`)،
  `POST /api/calibrate-level`,
  والإبقاء على `Set/Clear Home`.
  مع حظر حفظ إعدادات الطيران أثناء `ARMED`.
- توسيع `GET /api/settings` ليعيد كل الحقول اللازمة فعليًا لتحرير كامل `FlightSettings` (بما فيها حدود PID والمصفوفات والإزاحات) بدل النسخة الجزئية الحالية.
- إعادة تصميم `web_assets.h` إلى Dashboard مبسّط:
  واجهة أخف، عرض 3D أفضل بـ CSS (سلاسة/عمق/حركة أدق)، قسم GPS أبسط، ونموذج إعدادات منظم.
- إضافة i18n ثلاثي اللغة (AR/EN/ZH) مع تبديل فوري وحفظ الاختيار في `localStorage`.
- تنفيذ خرائط إنترنت تعمل داخل وخارج الصين عبر dual-provider strategy مع fallback رادار محلي عند فشل التحميل، وتخفيف معدل إعادة رسم الخريطة لتحسين السلاسة.
- تنظيف العناصر غير الضرورية من الواجهة الحالية (بطاقات/نصوص معلوماتية زائدة) مع الإبقاء على الوظائف المطلوبة فقط.
- تعديل `platformio.ini` إلى `build_src_filter` انتقائي لتجميع ملفات التطبيق المطلوبة فقط بدل تجميع كل `src/drone/**`، مع الإبقاء على مكتبات `Espfc` في المشروع للاستفادة المستهدفة منها.

### Public Interfaces / API Changes

- `GpsManager` API داخل المشروع سيحصل على واجهة متوافقة مع `FlightController` + تمكين/تعطيل GPS.
- `TelemetrySnapshot` سيُوسّع بحقول حالة GPS التشغيلية اللازمة للواجهة.
- `GET /api/settings` سيُرجع schema أغنى لتمكين تعديل كل الإعدادات.
- `POST /api/settings` سيتحول من read-only إلى write-enabled مع قيود أمان (`ARMED` block).

### Test Plan

- تحقق بناء المشروع (PlatformIO) بعد إدخال `gps_manager` وضبط `build_src_filter`.
- Smoke API tests:
  `GET /api/state`, `GET /api/settings`, `POST /api/settings`, `POST /api/calibrate-level`, `POST /api/home/set`, `POST /api/home/clear`.
- اختبار أمان: محاولة `POST /api/settings` أثناء `ARMED` يجب أن تُرفض برسالة واضحة.
- اختبار GPS:
  تفعيل/تعطيل عبر `SENS`, فقدان RC، بقاء `Home` محفوظة، وصحة `fix/home/distance/bearing`.
- اختبار UI:
  تبديل اللغات الثلاث، سلاسة 3D، عمل الخرائط داخل/خارج الصين، fallback للرادار عند انقطاع الإنترنت.
- اختبار Regression سريع: RC bars، motor outputs، تحديثات IMU، وعدم كسر TFT/status.

### Assumptions & Defaults

- الاعتماد على ملفات GPS الخارجية التي تم تحديدها كمصدر أساسي ثم تكييفها داخل المشروع الحالي.
- قناة التحكم بـ GPS هي `SENS (CH7)`.
- شرط fix الافتراضي `Min 5 satellites`.
- خرائط dual-provider لتعمل داخل وخارج الصين، مع fallback محلي دائم.
- الواجهة تفعّل أزرار: حفظ الإعدادات + معايرة المستوى + Home set/clear فقط.
- لم أستطع تشغيل build في البيئة الحالية لأن أوامر `pio/platformio` غير متاحة هنا؛ التحقق البنائي النهائي سيكون على بيئة فيها PlatformIO.
