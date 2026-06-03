#pragma once

#include <Arduino.h>
#include "UserConfig.h"
#include "ScaleManager.h"
#include "TapInput.h"

enum class ReminderState {
  BottleMissing,
  PausedManual,
  PausedAuto,
  Meeting,
  RefillDetected,
  JustDrank,
  Quiet,
  Ramp,
  Due,
  Overdue,
  Complete,
  DayComplete
};

enum class SipQuality {
  None,
  TooSmall,
  Good,
  Extra
};

struct HydrationStatus {
  ReminderState state = ReminderState::BottleMissing;
  uint16_t consumedMl = 0;
  uint16_t targetMl = 0;
  float progress = 0.0f;
  float reminderIntensity = 0.0f;
  float expectedMl = 0.0f;
  float deficitMl = 0.0f;
  int16_t balanceMl = 0;
  uint16_t recommendedSipMl = 0;
  bool overdue = false;
  bool vibrationAllowed = true;
  bool sipDetected = false;
  bool baselineReady = false;
  bool baselineCaptured = false;
  bool refillDetected = false;
  bool bottleChangeDetected = false;
  uint16_t lastSipMl = 0;
  uint16_t lastSipReferenceMl = 0;
  SipQuality lastSipQuality = SipQuality::None;
  uint8_t profileIndex = 0;
  const char* profileName = "";
};

class HydrationTracker {
public:
  void begin(unsigned long nowMs);
  void handleCommand(TapCommand command, unsigned long nowMs);
  HydrationStatus update(const ScaleReading& reading, unsigned long nowMs);
  HydrationStatus status() const;
  void resetSession(unsigned long nowMs);

private:
  const DrinkProfile& profile() const;
  uint16_t targetMl() const;
  uint16_t durationMin() const;
  uint16_t expectedSipMl() const;
  uint16_t mlFromGrams(float grams) const;
  uint16_t recommendedSip(float deficitMl, unsigned long activeElapsedMs) const;
  void commitSip(uint16_t sipMl, unsigned long nowMs);
  void captureBottleBaseline(float grams, bool captured = false);
  void noteActivity(unsigned long nowMs);
  void restartAfterIdleIfNeeded(unsigned long nowMs);
  void setPaused(bool paused, bool automatic, unsigned long nowMs);
  unsigned long activeElapsedMs(unsigned long nowMs) const;
  void updatePlanFields(unsigned long nowMs);

  HydrationStatus _status;
  bool _wasPresent = false;
  bool _hasStableBottleWeight = false;
  float _stableBottleWeightGrams = 0.0f;
  unsigned long _sessionStartMs = 0;
  unsigned long _lastSipMs = 0;
  unsigned long _justDrankUntilMs = 0;
  unsigned long _refillUntilMs = 0;
  unsigned long _missingSinceMs = 0;
  unsigned long _pauseStartedMs = 0;
  unsigned long _totalPausedMs = 0;
  unsigned long _meetingUntilMs = 0;
  unsigned long _lastActivityMs = 0;
  bool _manualPaused = false;
  bool _autoPaused = false;
  bool _dayComplete = false;
  uint8_t _profileIndex = DEFAULT_DRINK_PROFILE_INDEX;
};
