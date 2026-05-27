#include "WIFI/wifi_manager.h"

#include <WiFi.h>
#include <string.h>

#include "INPUT/drone_config.h"

namespace {

FlightSettings g_settings{};
uint32_t g_last_sta_attempt_ms = 0;
bool g_sta_attempt_in_progress = false;

constexpr uint32_t kStationRetryMs = 30000;
constexpr uint32_t kStationConnectWindowMs = 12000;

bool StationSettingsChanged(const FlightSettings& current, const FlightSettings& next) {
  return current.sta_enabled != next.sta_enabled || strcmp(current.sta_ssid, next.sta_ssid) != 0 ||
         strcmp(current.sta_password, next.sta_password) != 0;
}

bool HasStaCredentials(const FlightSettings& settings) {
  return settings.sta_enabled && settings.sta_ssid[0] != '\0' &&
         (settings.sta_password[0] == '\0' || strlen(settings.sta_password) >= 8);
}

bool AccessPointSettingsChanged(const FlightSettings& current, const FlightSettings& next) {
  return strcmp(current.ap_ssid, next.ap_ssid) != 0 ||
         strcmp(current.ap_password, next.ap_password) != 0;
}

void ConfigureWifiMode(const FlightSettings& settings) {
  WiFi.persistent(false);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  WiFi.mode(HasStaCredentials(settings) ? WIFI_AP_STA : WIFI_AP);
}

void StopStation(bool erase_config) {
  WiFi.disconnect(false, erase_config);
  g_last_sta_attempt_ms = millis();
  g_sta_attempt_in_progress = false;
}

bool StationTargetMatches(const FlightSettings& settings) {
  if (WiFi.SSID().isEmpty()) return false;
  return WiFi.SSID().equals(settings.sta_ssid);
}

void StartStationAttempt(const FlightSettings& settings, bool force_restart) {
  if (!HasStaCredentials(settings)) {
    StopStation(false);
    return;
  }

  const uint32_t now_ms = millis();
  if (!force_restart && g_sta_attempt_in_progress &&
      (now_ms - g_last_sta_attempt_ms) < kStationConnectWindowMs) {
    return;
  }

  if (!force_restart && WiFi.status() == WL_CONNECTED && StationTargetMatches(settings)) {
    g_sta_attempt_in_progress = false;
    return;
  }

  if (force_restart || !StationTargetMatches(settings)) {
    WiFi.disconnect(false, false);
    delay(40);
  }

  WiFi.begin(settings.sta_ssid, settings.sta_password);
  g_last_sta_attempt_ms = now_ms;
  g_sta_attempt_in_progress = true;
}

void StartNetworks(const FlightSettings& settings) {
  g_settings = settings;
  ConfigureWifiMode(settings);
  WiFi.softAPdisconnect(true);
  delay(50);

  const IPAddress ip(192, 168, 4, 1);
  const IPAddress gateway(192, 168, 4, 1);
  const IPAddress subnet(255, 255, 255, 0);

  WiFi.softAPConfig(ip, gateway, subnet);
  WiFi.softAP(settings.ap_ssid, settings.ap_password, DroneConfig::kWifiApChannel, false,
              DroneConfig::kWifiApMaxConnections);
  StartStationAttempt(settings, true);
}

}  // namespace

void WifiManager::beginAccessPoint(const FlightSettings& settings) {
  StartNetworks(settings);
}

void WifiManager::reconfigureAccessPoint(const FlightSettings& settings) {
  const bool restart_ap = AccessPointSettingsChanged(g_settings, settings);
  const bool restart_sta = StationSettingsChanged(g_settings, settings);
  if (restart_ap) {
    StartNetworks(settings);
    return;
  }

  g_settings = settings;
  ConfigureWifiMode(settings);
  if (!HasStaCredentials(settings)) {
    StopStation(false);
    return;
  }
  StartStationAttempt(settings, restart_sta);
}

void WifiManager::loop() {
  if (!HasStaCredentials(g_settings)) return;

  if (WiFi.status() == WL_CONNECTED) {
    g_sta_attempt_in_progress = false;
    return;
  }

  // Keep the AP stable while an operator is actively connected to it.
  if (WiFi.softAPgetStationNum() > 0) {
    return;
  }

  const uint32_t now_ms = millis();
  if (g_sta_attempt_in_progress && (now_ms - g_last_sta_attempt_ms) < kStationConnectWindowMs) {
    return;
  }
  if ((now_ms - g_last_sta_attempt_ms) < kStationRetryMs) return;
  StartStationAttempt(g_settings, true);
}

String WifiManager::accessPointIpAddress() {
  return WiFi.softAPIP().toString();
}

String WifiManager::stationIpAddress() {
  if (WiFi.status() != WL_CONNECTED) return String();
  return WiFi.localIP().toString();
}

String WifiManager::ipAddress() {
  const String sta_ip = stationIpAddress();
  if (sta_ip.length() > 0) return sta_ip;
  return accessPointIpAddress();
}

bool WifiManager::stationEnabled() {
  return HasStaCredentials(g_settings);
}

bool WifiManager::stationConnected() {
  return HasStaCredentials(g_settings) && WiFi.status() == WL_CONNECTED;
}

int32_t WifiManager::stationRssi() {
  if (WiFi.status() != WL_CONNECTED) return 0;
  return WiFi.RSSI();
}
