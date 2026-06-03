#include "ScaleManager.h"
#include "Config.h"
#include <math.h>

void ScaleManager::begin() {
  _scale.begin(PIN_HX711_DOUT, PIN_HX711_SCK);
  _scale.set_scale(SCALE_CALIBRATION_FACTOR);
  _scale.set_average_mode();
  delay(300);
  if (SCALE_TARE_ON_BOOT) {
    tareStable(6000UL);
  } else {
    _scale.set_offset(SCALE_ZERO_OFFSET);
  }
}

void ScaleManager::tare() {
  tareStable(4000UL);
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
  float raw = 0.0f;
  return sampleRawAverage(raw, samples, 3000UL) ? raw : _lastRaw;
}

float ScaleManager::readOffsetValue(uint8_t samples) {
  float raw = readRawAverage(samples);
  return raw - (float)_scale.get_offset();
}

float ScaleManager::readUnits(uint8_t samples) {
  float grams = 0.0f;
  return sampleUnitsAverage(grams, samples, 3000UL) ? grams : _reading.grams;
}

bool ScaleManager::isReady() {
  return _scale.is_ready();
}

bool ScaleManager::waitReady(unsigned long timeoutMs) {
  unsigned long startedAt = millis();
  while (millis() - startedAt < timeoutMs) {
    if (_scale.is_ready()) {
      return true;
    }
    delay(2);
    yield();
  }
  return false;
}

bool ScaleManager::readRawOnce(float& raw, unsigned long timeoutMs) {
  if (!waitReady(timeoutMs)) {
    return false;
  }
  raw = _scale.read();
  _lastRaw = raw;
  return true;
}

bool ScaleManager::sampleRawAverage(float& raw, uint8_t samples, unsigned long timeoutMs) {
  if (samples == 0) {
    samples = 1;
  }

  unsigned long startedAt = millis();
  float total = 0.0f;
  uint8_t count = 0;

  while (count < samples && millis() - startedAt < timeoutMs) {
    float sample = 0.0f;
    unsigned long remainingMs = timeoutMs - (millis() - startedAt);
    if (readRawOnce(sample, min(remainingMs, 350UL))) {
      total += sample;
      count++;
    } else {
      delay(2);
    }
    yield();
  }

  if (count == 0) {
    return false;
  }

  raw = total / (float)count;
  return count >= samples;
}

bool ScaleManager::sampleUnitsAverage(float& grams, uint8_t samples, unsigned long timeoutMs) {
  float raw = 0.0f;
  if (!sampleRawAverage(raw, samples, timeoutMs)) {
    return false;
  }

  float units = (raw - (float)_scale.get_offset()) / _scale.get_scale();
  grams = SCALE_INVERT_SIGN ? -units : units;
  return true;
}

bool ScaleManager::tareStable(unsigned long timeoutMs) {
  float raw = 0.0f;
  if (!sampleRawAverage(raw, 20, timeoutMs)) {
    return false;
  }

  _scale.set_offset((int32_t)raw);
  return true;
}

void ScaleManager::applyRawSample(float raw, unsigned long nowMs) {
  _lastRaw = raw;
  float units = (raw - (float)_scale.get_offset()) / _scale.get_scale();
  float instantGrams = SCALE_INVERT_SIGN ? -units : units;
  if (!_filterInitialized) {
    _filteredGrams = instantGrams;
    _filterInitialized = true;
  } else {
    _filteredGrams += SCALE_FILTER_ALPHA * (instantGrams - _filteredGrams);
  }

  float delta = fabs(_filteredGrams - _lastGrams);
  _lastGrams = _filteredGrams;
  _lastGoodSampleMs = nowMs;

  if (delta <= STABLE_DELTA_GRAMS) {
    if (_stableSamples < STABLE_REQUIRED_SAMPLES) {
      _stableSamples++;
    }
  } else {
    _stableSamples = 0;
  }

  if (_reading.bottlePresent) {
    if (_filteredGrams <= BOTTLE_REMOVE_GRAMS) {
      if (_missingSamples < BOTTLE_MISSING_REQUIRED_SAMPLES) {
        _missingSamples++;
      }
    } else {
      _missingSamples = 0;
    }

    if (_missingSamples >= BOTTLE_MISSING_REQUIRED_SAMPLES) {
      _reading.bottlePresent = false;
      _reading.stable = false;
      _stableSamples = 0;
      _missingSamples = 0;
      _presentSamples = 0;
    }
  } else {
    // A tap on the empty puck can briefly exceed the bottle threshold.
    // Confirm placement only after the instantaneous and filtered readings converge.
    bool settledBottleCandidate =
      _filteredGrams >= BOTTLE_PRESENT_GRAMS &&
      delta <= BOTTLE_PRESENT_SETTLE_DELTA_GRAMS &&
      fabs(instantGrams - _filteredGrams) <= BOTTLE_PRESENT_SETTLE_DELTA_GRAMS;

    if (settledBottleCandidate) {
      if (_presentSamples < BOTTLE_PRESENT_REQUIRED_SAMPLES) {
        _presentSamples++;
      }
    } else {
      _presentSamples = 0;
    }

    if (_presentSamples >= BOTTLE_PRESENT_REQUIRED_SAMPLES) {
      _reading.bottlePresent = true;
      _reading.stable = false;
      _stableSamples = 0;
      _presentSamples = 0;
      _missingSamples = 0;
    }
  }

  _reading.grams = _filteredGrams;
  _reading.instantGrams = instantGrams;
  _reading.stable = _stableSamples >= STABLE_REQUIRED_SAMPLES;
  _reading.valid = true;
  _reading.fresh = true;
  _reading.ageMs = 0;
}

ScaleReading ScaleManager::update(unsigned long nowMs) {
  _reading.fresh = false;
  if (_reading.valid) {
    _reading.ageMs = nowMs - _lastGoodSampleMs;
    if (_reading.ageMs > SCALE_STALE_AFTER_MS) {
      _reading.valid = false;
      _reading.stable = false;
      _stableSamples = 0;
    }
  }

  if (nowMs - _lastSampleMs < SCALE_SAMPLE_MS) {
    return _reading;
  }

  _lastSampleMs = nowMs;

  if (!_scale.is_ready()) {
    return _reading;
  }

  float raw = 0.0f;
  if (!readRawOnce(raw, 5UL)) {
    return _reading;
  }

  applyRawSample(raw, nowMs);
  return _reading;
}
