#pragma once

#include <Arduino.h>

// DrinkLight hardware and low-level tuning.
// Change this file when wiring changes, after mechanical changes, or during calibration.

static const uint8_t PIN_NEOPIXEL = 25;
static const uint8_t PIN_HX711_DOUT = 21;
static const uint8_t PIN_HX711_SCK = 22;
static const uint8_t PIN_VIBRATION = 5;

static const uint16_t LED_COUNT = 8;

// HX711 calibration. Replace with your measured value after calibration.
// Positive or negative depends on how the load cell is mounted.
static const float SCALE_CALIBRATION_FACTOR = 641.772827f;
static const bool SCALE_INVERT_SIGN = false;           // true if weight becomes negative when loaded.

// Zero handling.
// Recommended for the final product:
// 1. set SCALE_TARE_ON_BOOT to true during setup;
// 2. boot with the empty puck and read tare_offset from Serial Monitor;
// 3. paste that value into SCALE_ZERO_OFFSET;
// 4. set SCALE_TARE_ON_BOOT to false.
static const bool SCALE_TARE_ON_BOOT = false;
static const int32_t SCALE_ZERO_OFFSET = 37563;

// Weight filtering and bottle detection.
static const float BOTTLE_PRESENT_GRAMS = 120.0f;
static const float BOTTLE_REMOVE_GRAMS = 90.0f;
static const uint8_t BOTTLE_PRESENT_REQUIRED_SAMPLES = 8;
static const uint8_t BOTTLE_MISSING_REQUIRED_SAMPLES = 4;
static const float BOTTLE_PRESENT_SETTLE_DELTA_GRAMS = 8.0f;
static const float SCALE_FILTER_ALPHA = 0.28f;
static const float STABLE_DELTA_GRAMS = 3.0f;
static const uint16_t STABLE_REQUIRED_SAMPLES = 8;
static const uint16_t SCALE_SAMPLE_MS = 120;
static const uint16_t SCALE_STALE_AFTER_MS = 2500;

// Sip/refill recognition thresholds.
static const uint16_t MIN_SIP_ML = 15;                // Ignore tiny noise.
static const uint16_t MAX_SIP_ML = 350;               // Ignore removal anomalies.
static const uint16_t REFILL_DELTA_ML = 120;          // Bigger weight gain means refill.
static const uint16_t BOTTLE_CHANGE_DELTA_ML = 450;   // Big drop means new/empty bottle, not a sip.

// Weight-tap gesture detection. A tap is a quick vertical push on the puck.
static const float TAP_DELTA_GRAMS = 22.0f;
static const float TAP_MAX_DELTA_GRAMS = 600.0f;
static const float TAP_RELEASE_DELTA_GRAMS = 14.0f;
static const uint16_t TAP_SEQUENCE_GAP_MS = 800;
static const uint16_t TAP_SEQUENCE_MAX_MS = 6500;
static const uint16_t TAP_REFRACTORY_MS = 100;

// Vibration motor timing.
static const uint16_t VIBRATION_PULSE_MS = 180;
static const uint16_t VIBRATION_REPEAT_SEC = 55;
