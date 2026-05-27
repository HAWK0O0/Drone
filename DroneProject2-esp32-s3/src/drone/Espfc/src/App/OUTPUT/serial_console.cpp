#include "OUTPUT/serial_console.h"

#include <Arduino.h>

namespace SerialConsole {

void begin() {
  Serial.begin(115200);
  delay(150);
}

void printBootBanner(const FlightSettings& settings) {
  Serial.println();
  Serial.println(F("=== ESP32-S3 Drone FC App ==="));
  Serial.print(F("AP SSID: "));
  Serial.println(settings.ap_ssid);
  Serial.println(F("RC map: CH1=Roll, CH2=Pitch, CH3=Throttle, CH4=Yaw"));
  Serial.println(F("Motor map: M1=Front-Left, M2=Front-Right, M3=Rear-Left, M4=Rear-Right"));
}

void reportDrivers(bool motor_output_ready, bool imu_ready, bool gps_ready) {
  Serial.print(F("Drivers | motor="));
  Serial.print(motor_output_ready ? F("OK") : F("ERR"));
  Serial.print(F(" imu="));
  Serial.print(imu_ready ? F("OK") : F("ERR"));
  Serial.print(F(" gps="));
  Serial.println(gps_ready ? F("OK") : F("WAIT"));
}

}  // namespace SerialConsole
