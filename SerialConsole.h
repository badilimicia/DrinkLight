#pragma once

#include <Arduino.h>
#include "HydrationTracker.h"
#include "RemoteServices.h"
#include "ScaleManager.h"
#include "TapInput.h"
#include "VibrationMotor.h"
#include "LedRing.h"

class SerialConsole {
public:
  void begin(ScaleManager& scale, RemoteServices* remote);
  TapCommand update(
    const ScaleReading& reading,
    const HydrationStatus& status,
    ScaleManager& scale,
    LedRing& ledRing,
    VibrationMotor& vibration,
    unsigned long nowMs);
  void printPeriodicStatus(
    const ScaleReading& reading,
    const HydrationStatus& status,
    TapCommand command,
    unsigned long nowMs);
  void printDeducedEvents(
    const ScaleReading& reading,
    const HydrationStatus& status,
    TapCommand command);
  void printTapPulseDetected(uint8_t tapCount);
  void printTapSequenceDetected(uint8_t tapCount);
  void printCommandDetected(TapCommand command);

  static bool testSerialEnabled();
  static bool testHardwareEnabled();

private:
  enum class CalibrationGuideState {
    Idle,
    WaitWeight,
    WaitSettle,
    Sampling
  };

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
  const __FlashStringHelper* reminderStateName(ReminderState state) const;
  const __FlashStringHelper* sipQualityName(SipQuality quality) const;
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
  void startGuidedCalibration(unsigned long nowMs);
  bool handleGuidedCalibrationInput(ScaleManager& scale, LedRing& ledRing, const char* command, unsigned long nowMs);
  void updateGuidedCalibration(ScaleManager& scale, LedRing& ledRing, unsigned long nowMs);
  void resetGuidedCalibration();
  void finishGuidedCalibration(ScaleManager& scale, LedRing& ledRing, unsigned long nowMs);

  char _line[48] = {0};
  uint8_t _lineLength = 0;
  unsigned long _lastDebugMs = 0;
  unsigned long _calibrationGuideAtMs = 0;
  bool _remoteEchoEnabled = false;
  bool _hasCalibrationPoint = false;
  bool _eventStateInitialized = false;
  bool _lastBottlePresent = false;
  bool _lastReadingValid = false;
  ReminderState _lastReminderState = ReminderState::BottleMissing;
  CalibrationGuideState _calibrationGuideState = CalibrationGuideState::Idle;
  static constexpr uint8_t CALIBRATION_GUIDE_POINTS = 3;
  uint8_t _calibrationGuideIndex = 0;
  float _calibrationPointGrams = 0.0f;
  float _calibrationPointRaw = 0.0f;
  float _calibrationGuideGrams[CALIBRATION_GUIDE_POINTS] = {0.0f, 0.0f, 0.0f};
  float _calibrationGuideRaw[CALIBRATION_GUIDE_POINTS] = {0.0f, 0.0f, 0.0f};
  RemoteServices* _remote = NULL;
};
