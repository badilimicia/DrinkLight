#pragma once

#include <Arduino.h>
#include <HX711.h>

struct ScaleReading {
  float grams = 0.0f;
  bool bottlePresent = false;
  bool stable = false;
  bool valid = false;
};

class ScaleManager {
public:
  void begin();
  ScaleReading update(unsigned long nowMs);
  void tare();
  int32_t tareOffset();
  float scaleFactor();
  void setScaleFactor(float scaleFactor);
  void setZeroOffset(int32_t offset);
  float readRawAverage(uint8_t samples);
  float readOffsetValue(uint8_t samples);
  float readUnits(uint8_t samples);
  bool isReady();

private:
  HX711 _scale;
  ScaleReading _reading;
  unsigned long _lastSampleMs = 0;
  float _lastGrams = 0.0f;
  uint16_t _stableSamples = 0;
};
