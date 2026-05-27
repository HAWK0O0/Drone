#pragma once

#include "INPUT/drone_types.h"

namespace SettingsStore {

FlightSettings Defaults();
FlightSettings Load();
bool Save(const FlightSettings& settings);
void Sanitize(FlightSettings* settings);

}  // namespace SettingsStore
