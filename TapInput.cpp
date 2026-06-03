#include "TapInput.h"
#include "Config.h"
#include <math.h>

TapCommand TapInput::update(const ScaleReading& reading, unsigned long nowMs) {
  if (!TAP_COMMANDS_ENABLED || !reading.valid) {
    resetDetection();
    return TapCommand::None;
  }

  if (!inputAllowed(reading)) {
    resetDetection();
    return TapCommand::None;
  }

  if (_tapCount > 0 && nowMs - _lastTapMs >= TAP_SEQUENCE_GAP_MS) {
    uint8_t count = _tapCount;
    _tapCount = 0;
    _completedTapCount = count;
    _sequenceReady = true;
    return commandForCount(count);
  }

  if (!reading.fresh) {
    return TapCommand::None;
  }

  if (!_hasLast) {
    if (!reading.stable) {
      return TapCommand::None;
    }
    _hasLast = true;
    _lastGrams = reading.instantGrams;
    _restGrams = reading.instantGrams;
    return TapCommand::None;
  }

  float stepDelta = fabs(reading.instantGrams - _lastGrams);
  float restDelta = fabs(reading.instantGrams - _restGrams);
  _lastGrams = reading.instantGrams;

  if (_tapPressed) {
    if (restDelta <= TAP_RELEASE_DELTA_GRAMS) {
      _tapPressed = false;
      _restGrams = reading.instantGrams;
    }
    return TapCommand::None;
  }

  bool pulse = (
    (restDelta >= TAP_DELTA_GRAMS || stepDelta >= TAP_DELTA_GRAMS) &&
    restDelta <= TAP_MAX_DELTA_GRAMS &&
    nowMs - _lastTapMs >= TAP_REFRACTORY_MS);

  if (pulse) {
    if (_tapCount == 0 || nowMs - _firstTapMs > TAP_SEQUENCE_MAX_MS) {
      _tapCount = 0;
      _firstTapMs = nowMs;
    }

    _tapCount++;
    _pulseTapCount = _tapCount;
    _pulseReady = true;
    _lastTapMs = nowMs;
    _tapPressed = true;
  } else if (_tapCount == 0 && restDelta <= TAP_RELEASE_DELTA_GRAMS) {
    // Follow slow drift only while idle. During a sequence the rest point stays fixed.
    _restGrams += 0.20f * (reading.instantGrams - _restGrams);
  }

  return TapCommand::None;
}

bool TapInput::takeDetectedPulse(uint8_t& count) {
  if (!_pulseReady) {
    return false;
  }

  count = _pulseTapCount;
  _pulseReady = false;
  return true;
}

bool TapInput::takeCompletedSequence(uint8_t& count) {
  if (!_sequenceReady) {
    return false;
  }

  count = _completedTapCount;
  _sequenceReady = false;
  return true;
}

bool TapInput::inputAllowed(const ScaleReading& reading) const {
  return TAP_ONLY_WHEN_BOTTLE_MISSING ? !reading.bottlePresent : reading.bottlePresent;
}

void TapInput::resetDetection() {
  _hasLast = false;
  _tapPressed = false;
  _pulseReady = false;
  _tapCount = 0;
}

TapCommand TapInput::commandForCount(uint8_t count) const {
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
  return TapCommand::None;
}
