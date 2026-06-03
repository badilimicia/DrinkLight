#pragma once

#include <Arduino.h>
#include "ScaleManager.h"

enum class TapCommand {
  None,
  ShowProgress,
  TogglePause,
  ToggleMeeting,
  LightShow,
  NextProfile,
  ResetSession,
  EndDay
};

class TapInput {
public:
  TapCommand update(const ScaleReading& reading, unsigned long nowMs);
  bool takeDetectedPulse(uint8_t& count);
  bool takeCompletedSequence(uint8_t& count);

private:
  bool inputAllowed(const ScaleReading& reading) const;
  void resetDetection();
  TapCommand commandForCount(uint8_t count) const;

  bool _hasLast = false;
  bool _tapPressed = false;
  bool _pulseReady = false;
  bool _sequenceReady = false;
  float _lastGrams = 0.0f;
  float _restGrams = 0.0f;
  uint8_t _tapCount = 0;
  uint8_t _pulseTapCount = 0;
  uint8_t _completedTapCount = 0;
  unsigned long _firstTapMs = 0;
  unsigned long _lastTapMs = 0;
};
