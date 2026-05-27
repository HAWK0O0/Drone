#pragma once

#include <Arduino.h>
#include "INPUT/drone_types.h"

namespace TftStatus {

void begin();
void update(const TelemetrySnapshot& telemetry);

}  // namespace TftStatus
