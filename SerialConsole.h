#pragma once

#include <Arduino.h>
#include "HydrationTracker.h"
#include "RemoteServices.h"
#include "ScaleManager.h"
#include "TapInput.h"
#include "VibrationMotor.h"

class SerialConsole {
public:
  void begin(ScaleManager& scale, RemoteServices* remote);
  TapCommand update(
    const ScaleReading& reading,
    const HydrationStatus& status,
    ScaleManager& scale,
    VibrationMotor& vibration,
    unsigned long nowMs);
  void printPeriodicStatus(
    const ScaleReading& reading,
    const HydrationStatus& status,
    TapCommand command,
    unsigned long nowMs);
  void printCommandDetected(TapCommand command);

  static bool testSerialEnabled();
  static bool testHardwareEnabled();

private:
  template <typename T>
  void print(const T& value) const {
    Serial.print(value);
    if (_remote != NULL && _remoteEchoEnabled) {
      _remote->print(value);
    }
  }

  template <typename T>
  void print(const T& value, int digits) const {
    Serial.print(value, digits);
    if (_remote != NULL && _remoteEchoEnabled) {
      _remote->print(value, digits);
    }
  }

  template <typename T>
  void println(const T& value) const {
    Serial.println(value);
    if (_remote != NULL && _remoteEchoEnabled) {
      _remote->println(value);
    }
  }

  template <typename T>
  void println(const T& value, int digits) const {
    Serial.println(value, digits);
    if (_remote != NULL && _remoteEchoEnabled) {
      _remote->println(value, digits);
    }
  }

  void println() const {
    Serial.println();
    if (_remote != NULL && _remoteEchoEnabled) {
      _remote->println();
    }
  }

  const __FlashStringHelper* commandName(TapCommand command) const;
  void printHelp() const;
  bool readLine(char* out, uint8_t outSize);
  bool commandEquals(const char* command, const char* shortName, const char* longName) const;
  float parseCommandNumber(const char* command) const;
  void normalizeCommand(char* command) const;
  void printStatus(const ScaleReading& reading, const HydrationStatus& status, TapCommand command) const;
  void printScaleSnapshot(ScaleManager& scale) const;
  void printRawScaleSnapshot(ScaleManager& scale) const;
  void runTareCommand(ScaleManager& scale) const;
  void runCalibrationCommand(ScaleManager& scale, float knownGrams) const;
  void runTwoPointCalibrationCommand(ScaleManager& scale, const char* command);

  char _line[48] = {0};
  uint8_t _lineLength = 0;
  unsigned long _lastDebugMs = 0;
  bool _remoteEchoEnabled = false;
  bool _hasCalibrationPoint = false;
  float _calibrationPointGrams = 0.0f;
  float _calibrationPointRaw = 0.0f;
  RemoteServices* _remote = NULL;
};
