#pragma once

#include "INPUT/drone_types.h"

namespace GpsManager {

void begin();
void setEnabled(bool enabled);
bool isEnabled();
void update();

GpsSnapshot snapshot();
bool setHomeToCurrent();
void clearHome();

}  // namespace GpsManager
