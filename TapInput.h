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

private:
  bool _hasLast = false;
  float _lastGrams = 0.0f;
  uint8_t _tapCount = 0;
  unsigned long _firstTapMs = 0;
  unsigned long _lastTapMs = 0;
};
