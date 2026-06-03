#pragma once

#include <Arduino.h>
#include <HX711.h>

struct ScaleReading {
  float grams = 0.0f;
  float instantGrams = 0.0f;
  bool bottlePresent = false;
  bool stable = false;
  bool valid = false;
  bool fresh = false;
  unsigned long ageMs = 0;
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
  bool sampleRawAverage(float& raw, uint8_t samples, unsigned long timeoutMs);
  bool sampleUnitsAverage(float& grams, uint8_t samples, unsigned long timeoutMs);
  bool tareStable(unsigned long timeoutMs);
  bool isReady();

private:
  bool waitReady(unsigned long timeoutMs);
  bool readRawOnce(float& raw, unsigned long timeoutMs);
  void applyRawSample(float raw, unsigned long nowMs);

  HX711 _scale;
  ScaleReading _reading;
  unsigned long _lastSampleMs = 0;
  unsigned long _lastGoodSampleMs = 0;
  float _lastGrams = 0.0f;
  float _lastRaw = 0.0f;
  float _filteredGrams = 0.0f;
  bool _filterInitialized = false;
  uint8_t _presentSamples = 0;
  uint8_t _missingSamples = 0;
  uint16_t _stableSamples = 0;
};
