#include "ScaleManager.h"
#include "Config.h"
#include <math.h>

void ScaleManager::begin() {
  _scale.begin(PIN_HX711_DOUT, PIN_HX711_SCK);
  _scale.set_scale(SCALE_CALIBRATION_FACTOR);
  delay(300);
  if (SCALE_TARE_ON_BOOT) {
    tare();
  } else {
    _scale.set_offset(SCALE_ZERO_OFFSET);
  }
}

void ScaleManager::tare() {
  if (_scale.is_ready()) {
    _scale.tare(20);
  }
}

int32_t ScaleManager::tareOffset() {
  return _scale.get_offset();
}

float ScaleManager::scaleFactor() {
  return _scale.get_scale();
}

void ScaleManager::setScaleFactor(float scaleFactor) {
  _scale.set_scale(scaleFactor);
}

void ScaleManager::setZeroOffset(int32_t offset) {
  _scale.set_offset(offset);
}

float ScaleManager::readRawAverage(uint8_t samples) {
  if (!_scale.is_ready()) {
    return 0.0f;
  }
  return _scale.read_average(samples);
}

float ScaleManager::readOffsetValue(uint8_t samples) {
  if (!_scale.is_ready()) {
    return 0.0f;
  }
  return _scale.get_value(samples);
}

float ScaleManager::readUnits(uint8_t samples) {
  if (!_scale.is_ready()) {
    return 0.0f;
  }
  float grams = _scale.get_units(samples);
  return SCALE_INVERT_SIGN ? -grams : grams;
}

bool ScaleManager::isReady() {
  return _scale.is_ready();
}

ScaleReading ScaleManager::update(unsigned long nowMs) {
  if (nowMs - _lastSampleMs < SCALE_SAMPLE_MS) {
    return _reading;
  }

  _lastSampleMs = nowMs;

  if (!_scale.is_ready()) {
    _reading.valid = false;
    return _reading;
  }

  float grams = readUnits(3);
  float delta = fabs(grams - _lastGrams);
  _lastGrams = grams;

  if (delta <= STABLE_DELTA_GRAMS) {
    if (_stableSamples < STABLE_REQUIRED_SAMPLES) {
      _stableSamples++;
    }
  } else {
    _stableSamples = 0;
  }

  _reading.grams = grams;
  _reading.bottlePresent = grams >= BOTTLE_PRESENT_GRAMS;
  _reading.stable = _stableSamples >= STABLE_REQUIRED_SAMPLES;
  _reading.valid = true;

  return _reading;
}
