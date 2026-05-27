#pragma once

#include "INPUT/drone_types.h"

namespace SerialConsole {

void begin();
void printBootBanner(const FlightSettings& settings);
void reportDrivers(bool motor_output_ready, bool imu_ready, bool gps_ready);

}  // namespace SerialConsole
