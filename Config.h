#pragma once

// DrinkLight is now ESP32-only.
#ifndef ESP32
#error "DrinkLight targets ESP32 only. Select an ESP32 board in Arduino IDE."
#endif

// Compatibility include for the rest of the sketch.
// User-facing options live in UserConfig.h.
// Wiring, calibration and low-level thresholds live in HardwareConfig.h.

#include "UserConfig.h"
#include "HardwareConfig.h"
