#include "HydrationTracker.h"
#include "Config.h"

void HydrationTracker::begin(unsigned long nowMs) {
  resetSession(nowMs);
}

HydrationStatus HydrationTracker::status() const {
  return _status;
}

const DrinkProfile& HydrationTracker::profile() const {
  uint8_t safeIndex = _profileIndex < DRINK_PROFILE_COUNT ? _profileIndex : 0;
  return DRINK_PROFILES[safeIndex];
}

uint16_t HydrationTracker::targetMl() const {
  return profile().targetMl > 0 ? profile().targetMl : 1;
}

uint16_t HydrationTracker::durationMin() const {
  return profile().durationMin > 0 ? profile().durationMin : 1;
}

uint16_t HydrationTracker::expectedSipMl() const {
  return profile().referenceSipMl > 0 ? profile().referenceSipMl : 1;
}

void HydrationTracker::resetSession(unsigned long nowMs) {
  _status = HydrationStatus();
  _sessionStartMs = nowMs;
  _lastSipMs = nowMs;
  _justDrankUntilMs = 0;
  _refillUntilMs = 0;
  _missingSinceMs = 0;
  _pauseStartedMs = 0;
  _totalPausedMs = 0;
  _meetingUntilMs = 0;
  _lastActivityMs = nowMs;
  _manualPaused = false;
  _autoPaused = false;
  _dayComplete = false;
  _hasStableBottleWeight = false;
  _wasPresent = false;
  _status.targetMl = targetMl();
  _status.recommendedSipMl = expectedSipMl();
  _status.profileIndex = _profileIndex;
  _status.profileName = profile().name;
}

void HydrationTracker::noteActivity(unsigned long nowMs) {
  _lastActivityMs = nowMs;
}

void HydrationTracker::restartAfterIdleIfNeeded(unsigned long nowMs) {
  if (!AUTO_RESTART_SESSION_ENABLED || _lastActivityMs == 0) {
    return;
  }

  unsigned long idleLimitMs = (unsigned long)AUTO_RESTART_AFTER_IDLE_HOURS * 60UL * 60UL * 1000UL;
  if (idleLimitMs == 0) {
    return;
  }

  if (nowMs - _lastActivityMs >= idleLimitMs) {
    resetSession(nowMs);
  }
}

uint16_t HydrationTracker::mlFromGrams(float grams) const {
  if (grams <= 0.0f) {
    return 0;
  }
  return (uint16_t)(grams + 0.5f); // Water is close enough to 1 g/ml.
}

unsigned long HydrationTracker::activeElapsedMs(unsigned long nowMs) const {
  unsigned long pausedMs = _totalPausedMs;
  if ((_manualPaused || _autoPaused) && _pauseStartedMs > 0) {
    pausedMs += nowMs - _pauseStartedMs;
  }

  unsigned long elapsed = nowMs - _sessionStartMs;
  if (pausedMs >= elapsed) {
    return 0;
  }
  return elapsed - pausedMs;
}

uint16_t HydrationTracker::recommendedSip(float deficitMl, unsigned long activeElapsedMsValue) const {
  if (!USE_DYNAMIC_SIP_SIZE) {
    return expectedSipMl();
  }

  unsigned long sessionDurationMs = (unsigned long)durationMin() * 60UL * 1000UL;
  unsigned long remainingMs = sessionDurationMs > activeElapsedMsValue ? sessionDurationMs - activeElapsedMsValue : 1UL;
  float remainingTargetMl = (float)targetMl() - (float)_status.consumedMl;
  if (remainingTargetMl <= 0.0f) {
    return 0;
  }

  float intervalMs = ((float)expectedSipMl() * (float)sessionDurationMs) / (float)targetMl();
  float remainingSipSlots = (float)remainingMs / intervalMs;
  if (remainingSipSlots < 1.0f) {
    remainingSipSlots = 1.0f;
  }
  float baseSip = remainingTargetMl / remainingSipSlots;

  if (deficitMl > 0.0f) {
    // Recover delay across a few realistic sips instead of asking for one large drink.
    float recoverMl = deficitMl;
    if (recoverMl > (float)MAX_RECOMMENDED_SIP_ML) {
      recoverMl = (float)MAX_RECOMMENDED_SIP_ML;
    }
    baseSip += recoverMl / 3.0f;
  }

  return (uint16_t)constrain(baseSip, (float)MIN_RECOMMENDED_SIP_ML, (float)MAX_RECOMMENDED_SIP_ML);
}

void HydrationTracker::updatePlanFields(unsigned long nowMs) {
  unsigned long sessionDurationMs = (unsigned long)durationMin() * 60UL * 1000UL;
  unsigned long elapsedMs = min(activeElapsedMs(nowMs), sessionDurationMs);

  _status.targetMl = targetMl();
  _status.profileIndex = _profileIndex;
  _status.profileName = profile().name;
  _status.expectedMl = ((float)targetMl() * (float)elapsedMs) / (float)sessionDurationMs;
  _status.deficitMl = _status.expectedMl - (float)_status.consumedMl;
  _status.balanceMl = (int16_t)((float)_status.consumedMl - _status.expectedMl);
  _status.progress = min(1.0f, (float)_status.consumedMl / (float)targetMl());
  _status.recommendedSipMl = recommendedSip(_status.deficitMl, elapsedMs);
}

void HydrationTracker::commitSip(uint16_t sipMl, unsigned long nowMs) {
  // In static mode the sip amount is only feedback; it must not drive recovery.
  uint16_t recommended = USE_DYNAMIC_SIP_SIZE ? _status.recommendedSipMl : expectedSipMl();
  if (recommended == 0) {
    recommended = expectedSipMl();
  }

  uint32_t newTotal = (uint32_t)_status.consumedMl + sipMl;
  if (newTotal > targetMl()) {
    newTotal = targetMl();
  }
  _status.consumedMl = (uint16_t)newTotal;
  _status.lastSipMl = sipMl;
  _status.lastSipReferenceMl = recommended;
  _status.sipDetected = true;
  if (sipMl + 10 < recommended) {
    _status.lastSipQuality = SipQuality::TooSmall;
  } else if (sipMl > recommended + 40) {
    _status.lastSipQuality = SipQuality::Extra;
  } else {
    _status.lastSipQuality = SipQuality::Good;
  }
  _lastSipMs = nowMs;
  _justDrankUntilMs = nowMs + 3500UL;
  noteActivity(nowMs);
}

void HydrationTracker::captureBottleBaseline(float grams, bool captured) {
  _stableBottleWeightGrams = grams;
  _hasStableBottleWeight = true;
  _status.baselineReady = true;
  _status.baselineCaptured = captured;
}

void HydrationTracker::setPaused(bool paused, bool automatic, unsigned long nowMs) {
  bool wasPaused = _manualPaused || _autoPaused;

  if (automatic) {
    _autoPaused = paused;
  } else {
    _manualPaused = paused;
    if (!paused) {
      _autoPaused = false;
    }
  }

  bool isPaused = _manualPaused || _autoPaused;
  if (!wasPaused && isPaused) {
    _pauseStartedMs = nowMs;
  } else if (wasPaused && !isPaused && _pauseStartedMs > 0) {
    _totalPausedMs += nowMs - _pauseStartedMs;
    _pauseStartedMs = 0;
    _lastSipMs = nowMs;
  }
}

void HydrationTracker::handleCommand(TapCommand command, unsigned long nowMs) {
  if (command == TapCommand::None) {
    return;
  }

  noteActivity(nowMs);

  if (command == TapCommand::TogglePause) {
    setPaused(!_manualPaused, false, nowMs);
    return;
  }

  if (command == TapCommand::ToggleMeeting) {
    if ((long)(nowMs - _meetingUntilMs) < 0) {
      _meetingUntilMs = 0;
    } else {
      _meetingUntilMs = nowMs + (unsigned long)MEETING_MODE_MIN * 60UL * 1000UL;
    }
    return;
  }

  if (command == TapCommand::NextProfile) {
    _profileIndex = (_profileIndex + 1) % DRINK_PROFILE_COUNT;
    _status.targetMl = targetMl();
    _status.recommendedSipMl = expectedSipMl();
    if (_status.consumedMl < targetMl()) {
      _dayComplete = false;
    }
    return;
  }

  if (command == TapCommand::ResetSession) {
    resetSession(nowMs);
    return;
  }

  if (command == TapCommand::EndDay) {
    _dayComplete = true;
    setPaused(false, false, nowMs);
  }
}

HydrationStatus HydrationTracker::update(const ScaleReading& reading, unsigned long nowMs) {
  restartAfterIdleIfNeeded(nowMs);

  _status.sipDetected = false;
  _status.baselineCaptured = false;
  _status.refillDetected = false;
  _status.bottleChangeDetected = false;
  _status.vibrationAllowed = true;
  updatePlanFields(nowMs);

  if (_dayComplete) {
    _status.state = ReminderState::DayComplete;
    _status.reminderIntensity = _status.progress;
    _status.overdue = false;
    _status.vibrationAllowed = false;
    return _status;
  }

  if (!reading.valid || !reading.bottlePresent) {
    _wasPresent = false;
    if (_missingSinceMs == 0) {
      _missingSinceMs = nowMs;
    }
    if (nowMs - _missingSinceMs >= (unsigned long)AUTO_PAUSE_MISSING_SEC * 1000UL) {
      setPaused(true, true, nowMs);
      _status.state = ReminderState::PausedAuto;
    } else {
      _status.state = ReminderState::BottleMissing;
    }
    _status.reminderIntensity = 0.0f;
    _status.overdue = false;
    _status.vibrationAllowed = false;
    return _status;
  }

  _missingSinceMs = 0;
  if (_autoPaused) {
    setPaused(false, true, nowMs);
  }

  if (reading.stable) {
    if (!_wasPresent) {
      // First stable bottle weight is the session baseline. It can be empty, half-full or full.
      if (_hasStableBottleWeight) {
        float deltaGrams = _stableBottleWeightGrams - reading.grams;
        float refillGrams = reading.grams - _stableBottleWeightGrams;
        if (refillGrams >= REFILL_DELTA_ML) {
          _refillUntilMs = nowMs + 2500UL;
          _status.refillDetected = true;
          captureBottleBaseline(reading.grams);
          noteActivity(nowMs);
        } else if (deltaGrams >= BOTTLE_CHANGE_DELTA_ML) {
          _status.bottleChangeDetected = true;
          captureBottleBaseline(reading.grams);
          noteActivity(nowMs);
        } else {
          uint16_t sipMl = mlFromGrams(deltaGrams);
          if (sipMl >= MIN_SIP_ML && sipMl <= MAX_SIP_ML) {
            commitSip(sipMl, nowMs);
            updatePlanFields(nowMs);
          }
          captureBottleBaseline(reading.grams);
        }
      } else {
        captureBottleBaseline(reading.grams, true);
        noteActivity(nowMs);
      }
    } else {
      captureBottleBaseline(reading.grams);
    }

    _wasPresent = true;
  }

  if (_manualPaused || _autoPaused) {
    _status.state = _manualPaused ? ReminderState::PausedManual : ReminderState::PausedAuto;
    _status.reminderIntensity = 0.0f;
    _status.overdue = false;
    _status.vibrationAllowed = false;
    return _status;
  }

  if ((long)(nowMs - _refillUntilMs) < 0) {
    _status.state = ReminderState::RefillDetected;
    _status.reminderIntensity = _status.progress;
    _status.overdue = false;
    return _status;
  }

  if (_status.consumedMl >= targetMl()) {
    _status.state = ReminderState::Complete;
    _status.reminderIntensity = 1.0f;
    _status.overdue = false;
    return _status;
  }

  if ((long)(nowMs - _justDrankUntilMs) < 0) {
    _status.state = ReminderState::JustDrank;
    _status.reminderIntensity = 0.0f;
    _status.overdue = false;
    return _status;
  }

  _status.lastSipQuality = SipQuality::None;

  unsigned long sessionDurationMs = (unsigned long)durationMin() * 60UL * 1000UL;
  float deficitMl = _status.deficitMl;
  uint16_t sipReferenceMl = USE_DYNAMIC_SIP_SIZE ? _status.recommendedSipMl : expectedSipMl();
  float sipProgress = deficitMl / (float)sipReferenceMl;
  _status.reminderIntensity = constrain(sipProgress, 0.0f, 1.0f);

  unsigned long rampMs = (unsigned long)RAMP_BEFORE_DUE_SEC * 1000UL;
  unsigned long graceMs = (unsigned long)OVERDUE_GRACE_SEC * 1000UL;
  unsigned long sipIntervalMs = ((unsigned long)expectedSipMl() * sessionDurationMs) / targetMl();
  unsigned long timeSinceSipMs = nowMs - _lastSipMs;

  if (!USE_DYNAMIC_SIP_SIZE) {
    if (timeSinceSipMs + rampMs < sipIntervalMs) {
      _status.state = (long)(nowMs - _meetingUntilMs) < 0 ? ReminderState::Meeting : ReminderState::Quiet;
      _status.reminderIntensity = 0.0f;
      _status.overdue = false;
      _status.vibrationAllowed = (long)(nowMs - _meetingUntilMs) >= 0;
      return _status;
    }

    unsigned long rampStart = sipIntervalMs > rampMs ? sipIntervalMs - rampMs : 0;
    _status.reminderIntensity = constrain((float)(timeSinceSipMs - rampStart) / (float)rampMs, 0.0f, 1.0f);
  } else if (deficitMl <= 0.0f) {
    _status.state = (long)(nowMs - _meetingUntilMs) < 0 ? ReminderState::Meeting : ReminderState::Quiet;
    _status.overdue = false;
    _status.vibrationAllowed = (long)(nowMs - _meetingUntilMs) >= 0;
    return _status;
  }

  if (timeSinceSipMs > sipIntervalMs + graceMs) {
    _status.state = (long)(nowMs - _meetingUntilMs) < 0 ? ReminderState::Meeting : ReminderState::Overdue;
    _status.reminderIntensity = 1.0f;
    _status.overdue = (long)(nowMs - _meetingUntilMs) >= 0;
    _status.vibrationAllowed = _status.overdue;
  } else if (timeSinceSipMs >= sipIntervalMs) {
    _status.state = (long)(nowMs - _meetingUntilMs) < 0 ? ReminderState::Meeting : ReminderState::Due;
    _status.reminderIntensity = 1.0f;
    _status.overdue = false;
    _status.vibrationAllowed = (long)(nowMs - _meetingUntilMs) >= 0;
  } else if (timeSinceSipMs + rampMs >= sipIntervalMs) {
    _status.state = (long)(nowMs - _meetingUntilMs) < 0 ? ReminderState::Meeting : ReminderState::Ramp;
    unsigned long rampStart = sipIntervalMs > rampMs ? sipIntervalMs - rampMs : 0;
    _status.reminderIntensity = constrain((float)(timeSinceSipMs - rampStart) / (float)rampMs, 0.0f, 1.0f);
    _status.overdue = false;
    _status.vibrationAllowed = (long)(nowMs - _meetingUntilMs) >= 0;
  } else {
    _status.state = (long)(nowMs - _meetingUntilMs) < 0 ? ReminderState::Meeting : ReminderState::Quiet;
    _status.reminderIntensity = 0.0f;
    _status.overdue = false;
    _status.vibrationAllowed = (long)(nowMs - _meetingUntilMs) >= 0;
  }

  return _status;
}
