#include "OUTPUT/tft_status.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include "INPUT/drone_config.h"

namespace {
// TFT hardware interface objects
// كائنات واجهة عتاد TFT
// 中文: TFT 硬件接口对象
SPIClass g_tft_spi(FSPI);
Adafruit_ST7789 g_tft(&g_tft_spi, DroneConfig::kTftCsPin, DroneConfig::kTftDcPin,
                      DroneConfig::kTftRstPin);

// Runtime state variables
// متغيرات حالة التشغيل
// 中文: 运行时状态变量
bool g_enabled = false;
uint32_t g_last_refresh_ms = 0;
constexpr uint16_t kDividerColor = 0x39E7;

// Helper function to draw text on TFT screen
// دالة مساعدة لرسم النص على شاشة TFT
// 中文: 在 TFT 屏幕上绘制文本的辅助函数
void DrawLine(const String& text, int16_t x, int16_t y, uint16_t color, uint8_t size) {
  g_tft.setTextColor(color);
  g_tft.setTextSize(size);
  g_tft.setCursor(x, y);
  g_tft.print(text);
}

String ClipText(const String& text, size_t max_len) {
  if (text.length() <= max_len) return text;
  if (max_len <= 3) return text.substring(0, max_len);
  return text.substring(0, max_len - 3) + "...";
}

String FormatGpsState(const TelemetrySnapshot& telemetry) {
  if (!telemetry.gps_ok) return String("NO DATA");
  if (telemetry.gps_fix) return String("FIX");
  return String("SEARCH");
}

String FormatStaState(const TelemetrySnapshot& telemetry) {
  if (!telemetry.sta_enabled) return String("DISABLED");
  if (!telemetry.sta_connected) return String("TRYING");
  return String("OK ") + String(telemetry.sta_rssi) + "dBm";
}

void DrawDivider(int16_t y, uint16_t color) {
  g_tft.drawFastHLine(8, y, g_tft.width() - 16, color);
}

void DrawKeyValue(const char* key, const String& value, int16_t x, int16_t y, uint16_t key_color,
                  uint16_t value_color) {
  DrawLine(String(key), x, y, key_color, 1);
  DrawLine(value, x + 48, y, value_color, 1);
}

const char* OnOffText(bool value) {
  return value ? "ON" : "OFF";
}

const char* OkWaitText(bool value, const char* ok_text, const char* wait_text) {
  return value ? ok_text : wait_text;
}

}  // namespace

void TftStatus::begin() {
  if (!DroneConfig::kEnableTft) {
    g_enabled = false;
    return;
  }

  // Initialize TFT hardware
  // تهيئة عتاد TFT
  // 中文: 初始化 TFT 硬件
  pinMode(DroneConfig::kTftBacklightPin, OUTPUT);
  digitalWrite(DroneConfig::kTftBacklightPin, HIGH);

  g_tft_spi.begin(DroneConfig::kTftSckPin, -1, DroneConfig::kTftMosiPin, DroneConfig::kTftCsPin);
  g_tft.init(DroneConfig::kTftWidth, DroneConfig::kTftHeight);
  g_tft.setRotation(DroneConfig::kTftRotation & 0x03);
  g_tft.fillScreen(ST77XX_BLACK);
  g_tft.setTextWrap(false);
  g_enabled = true;
}

void TftStatus::update(const TelemetrySnapshot& telemetry) {
  if (!g_enabled) return;
  const uint32_t now_ms = millis();
  const uint32_t refresh_period_ms =
      telemetry.armed ? DroneConfig::kTftRefreshArmedMs : DroneConfig::kTftRefreshMs;
  if ((now_ms - g_last_refresh_ms) < refresh_period_ms) return;
  g_last_refresh_ms = now_ms;

  g_tft.fillScreen(ST77XX_BLACK);

  const bool imu_cal_ready = telemetry.imu_ok && telemetry.imu_calibrated;
  const uint16_t status_color = telemetry.armed ? ST77XX_RED : ST77XX_GREEN;
  const uint16_t imu_color = imu_cal_ready ? ST77XX_GREEN : (telemetry.imu_ok ? ST77XX_ORANGE : ST77XX_RED);
  const uint16_t rc_color = telemetry.rc_ok ? ST77XX_GREEN : ST77XX_RED;
  const uint16_t gps_color = telemetry.gps_fix ? ST77XX_GREEN : (telemetry.gps_ok ? ST77XX_ORANGE : ST77XX_RED);
  const uint16_t sta_color =
      telemetry.sta_enabled ? (telemetry.sta_connected ? ST77XX_GREEN : ST77XX_ORANGE) : kDividerColor;

  DrawLine("DRONE FC", 10, 8, ST77XX_CYAN, 2);
  DrawLine(String(telemetry.armed ? "ARMED" : "SAFE"), 148, 8, status_color, 2);
  DrawLine(String(telemetry.mode_name), 10, 30, ST77XX_GREEN, 2);
  DrawLine(String("RC ") + (telemetry.rc_ok ? "OK" : "LOSS"), 130, 30, rc_color, 2);
  DrawDivider(50, kDividerColor);

  DrawKeyValue("IMU", OkWaitText(imu_cal_ready, "READY", telemetry.imu_ok ? "WAIT" : "ERR"), 10,
         58, ST77XX_WHITE, imu_color);
  DrawKeyValue("ARM", OnOffText(telemetry.arm_switch_active), 126, 58, ST77XX_WHITE,
         telemetry.arm_switch_active ? ST77XX_GREEN : ST77XX_ORANGE);
  DrawKeyValue("STAB", OnOffText(telemetry.sensor_switch_active), 10, 74, ST77XX_WHITE,
         telemetry.sensor_switch_active ? ST77XX_GREEN : ST77XX_ORANGE);
  DrawKeyValue("CAL",
         "S" + String(telemetry.imu_cal_sys) + " G" + String(telemetry.imu_cal_gyro) +
           " A" + String(telemetry.imu_cal_accel) + " M" + String(telemetry.imu_cal_mag),
         74, 74, ST77XX_WHITE, ST77XX_YELLOW);

  DrawDivider(92, kDividerColor);

    DrawKeyValue("GPS", FormatGpsState(telemetry) + " SAT " + String(telemetry.gps_satellites), 10,
           100, ST77XX_WHITE, gps_color);
    DrawLine("WIFI", 10, 116, ST77XX_CYAN, 1);
    DrawKeyValue("AP", ClipText(String(telemetry.ap_ssid), 16), 10, 128, ST77XX_WHITE, ST77XX_WHITE);
    DrawKeyValue("AP IP", String(telemetry.ap_ip_address), 10, 142, ST77XX_WHITE, ST77XX_YELLOW);
    DrawKeyValue("STA", ClipText(FormatStaState(telemetry), 16), 10, 156, ST77XX_WHITE, sta_color);
    DrawKeyValue("STA IP", telemetry.sta_connected ? String(telemetry.sta_ip_address) : String("--"), 10,
           170, ST77XX_WHITE, telemetry.sta_connected ? ST77XX_GREEN : ST77XX_WHITE);
    DrawKeyValue("IP",
           ClipText(String(telemetry.sta_connected ? telemetry.sta_ip_address : telemetry.ap_ip_address),
              16),
           10, 184, ST77XX_WHITE, ST77XX_CYAN);

    DrawDivider(200, kDividerColor);

    DrawLine(String("ROLL ") + String(telemetry.roll_deg, 1) + "  PITCH " +
           String(telemetry.pitch_deg, 1),
         10, 208, ST77XX_WHITE, 1);
    DrawLine(String("YAW  ") + String(telemetry.yaw_deg, 1) + "  THR " +
           String(telemetry.throttle_percent, 0) + "%",
         10, 222, ST77XX_WHITE, 1);
}
