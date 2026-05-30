#include "TapInput.h"
#include "Config.h"
#include <math.h>

TapCommand TapInput::update(const ScaleReading& reading, unsigned long nowMs) {
  if (!TAP_COMMANDS_ENABLED) {
    return TapCommand::None;
  }

  if (!reading.valid || !reading.bottlePresent) {
    _hasLast = false;
    _tapCount = 0;
    return TapCommand::None;
  }

  if (!_hasLast) {
    _hasLast = true;
    _lastGrams = reading.grams;
    return TapCommand::None;
  }

  float delta = fabs(reading.grams - _lastGrams);
  _lastGrams = reading.grams;

  bool pulse = delta >= TAP_DELTA_GRAMS && delta <= TAP_MAX_DELTA_GRAMS;
  if (pulse && nowMs - _lastTapMs >= TAP_REFRACTORY_MS) {
    if (_tapCount == 0 || nowMs - _firstTapMs > TAP_WINDOW_MS) {
      _tapCount = 0;
      _firstTapMs = nowMs;
    }

    _tapCount++;
    _lastTapMs = nowMs;
  }

  if (_tapCount > 0 && nowMs - _firstTapMs > TAP_WINDOW_MS) {
    uint8_t count = _tapCount;
    _tapCount = 0;

    if (count == TAP_COUNT_PROGRESS) {
      return TapCommand::ShowProgress;
    }
    if (count == TAP_COUNT_PAUSE) {
      return TapCommand::TogglePause;
    }
    if (count == TAP_COUNT_MEETING) {
      return TapCommand::ToggleMeeting;
    }
    if (count == TAP_COUNT_LIGHT_SHOW) {
      return TapCommand::LightShow;
    }
    if (count == TAP_COUNT_NEXT_PROFILE) {
      return TapCommand::NextProfile;
    }
    if (count == TAP_COUNT_RESET_SESSION) {
      return TapCommand::ResetSession;
    }
    if (count >= TAP_COUNT_END_DAY) {
      return TapCommand::EndDay;
    }
  }

  return TapCommand::None;
}
