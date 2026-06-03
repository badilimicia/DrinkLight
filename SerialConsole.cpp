#include "SerialConsole.h"
#include "Config.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

bool SerialConsole::testSerialEnabled() {
  return TEST_MODE == TestMode::Serial || TEST_MODE == TestMode::Full;
}

bool SerialConsole::testHardwareEnabled() {
  return TEST_MODE == TestMode::Hardware || TEST_MODE == TestMode::Full;
}

void SerialConsole::begin(ScaleManager& scale, RemoteServices* remote) {
  _remote = remote;

  println(F("DrinkLight ready"));
  print(F("scale_tare_on_boot="));
  println(SCALE_TARE_ON_BOOT);
  print(F("scale_offset="));
  println(scale.tareOffset());

  if (TEST_MODE != TestMode::Off) {
    println(F("TEST_MODE enabled"));
    println(F("tap map: 3=progress 4=pause 5=meeting 6=light_show 7=profile 8=reset 9+=end_day"));
  }

  printHelp();
}

const __FlashStringHelper* SerialConsole::commandName(TapCommand command) const {
  switch (command) {
    case TapCommand::ShowProgress:
      return F("show_progress");
    case TapCommand::TogglePause:
      return F("toggle_pause");
    case TapCommand::ToggleMeeting:
      return F("toggle_meeting");
    case TapCommand::LightShow:
      return F("light_show");
    case TapCommand::NextProfile:
      return F("next_profile");
    case TapCommand::ResetSession:
      return F("reset_session");
    case TapCommand::EndDay:
      return F("end_day");
    case TapCommand::None:
      return F("none");
  }
  return F("unknown");
}

const __FlashStringHelper* SerialConsole::reminderStateName(ReminderState state) const {
  switch (state) {
    case ReminderState::BottleMissing:
      return F("bottle_missing");
    case ReminderState::PausedManual:
      return F("paused_manual");
    case ReminderState::PausedAuto:
      return F("paused_auto");
    case ReminderState::Meeting:
      return F("meeting");
    case ReminderState::RefillDetected:
      return F("refill_detected");
    case ReminderState::JustDrank:
      return F("just_drank");
    case ReminderState::Quiet:
      return F("quiet");
    case ReminderState::Ramp:
      return F("ramp");
    case ReminderState::Due:
      return F("due");
    case ReminderState::Overdue:
      return F("overdue");
    case ReminderState::Complete:
      return F("complete");
    case ReminderState::DayComplete:
      return F("day_complete");
  }
  return F("unknown");
}

const __FlashStringHelper* SerialConsole::sipQualityName(SipQuality quality) const {
  switch (quality) {
    case SipQuality::None:
      return F("none");
    case SipQuality::TooSmall:
      return F("small");
    case SipQuality::Good:
      return F("ok");
    case SipQuality::Extra:
      return F("large");
  }
  return F("unknown");
}

void SerialConsole::printHelp() const {
  println(F("commands:"));
  println(F("  h/help/?      show this help"));
  println(F("  s/status      print current status"));
  println(F("  p/progress    show progress on LEDs"));
  println(F("  a/pause       toggle pause"));
  println(F("  m/meeting     toggle meeting mode"));
  println(F("  l/light       run light show"));
  println(F("  n/profile     next profile"));
  println(F("  r/reset       reset session counter"));
  println(F("  e/end         end day"));
  println(F("  v/vib         short vibration pulse"));
  println(F("  g/grams       print current grams"));
  println(F("  raw           print raw HX711 values"));
  println(F("  tare/zero     tare now and print SCALE_ZERO_OFFSET"));
  println(F("  cal <grams>   calibrate scale factor with known weight"));
  println(F("  cal2 <grams>  two-point calibration, run once per known weight"));
  println(F("  calauto       guided 3-point calibration with LED feedback"));
  println(F("  cal2 reset    clear pending two-point calibration"));
  println(F("Example: tare, then put 500g, then cal 500"));
  println(F("Example: put 0g -> cal2 0, put 500g -> cal2 500"));
  println(F("calauto tip: use 0g, a medium weight, then a higher weight."));
}

bool SerialConsole::readLine(char* out, uint8_t outSize) {
  if (_remote != NULL && _remote->readLine(out, outSize)) {
    _remoteEchoEnabled = true;
    return true;
  }

  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r' || c == '\n') {
      _line[_lineLength] = '\0';
      strncpy(out, _line, outSize);
      out[outSize - 1] = '\0';
      _lineLength = 0;
      _remoteEchoEnabled = false;
      return out[0] != '\0';
    }

    if (c == '\b' || c == 127) {
      if (_lineLength > 0) {
        _lineLength--;
      }
      continue;
    }

    if (c >= 32 && c < 127 && _lineLength < sizeof(_line) - 1) {
      _line[_lineLength++] = c;
    }
  }

  return false;
}

bool SerialConsole::commandEquals(const char* command, const char* shortName, const char* longName) const {
  return strcmp(command, shortName) == 0 || strcmp(command, longName) == 0;
}

float SerialConsole::parseCommandNumber(const char* command) const {
  const char* space = strchr(command, ' ');
  if (space == NULL) {
    return 0.0f;
  }
  return atof(space + 1);
}

void SerialConsole::normalizeCommand(char* command) const {
  for (uint8_t i = 0; command[i] != '\0'; i++) {
    if (command[i] >= 'A' && command[i] <= 'Z') {
      command[i] = command[i] + ('a' - 'A');
    }
  }
}

void SerialConsole::printScaleSnapshot(ScaleManager& scale) const {
  float grams = 0.0f;
  bool sampled = scale.sampleUnitsAverage(grams, 5, 2000UL);
  print(F("ready="));
  print(sampled);
  print(F(" grams="));
  print(grams, 2);
  print(F(" scale_factor="));
  print(scale.scaleFactor(), 6);
  print(F(" offset="));
  println(scale.tareOffset());
}

void SerialConsole::printRawScaleSnapshot(ScaleManager& scale) const {
  float raw = 0.0f;
  float offsetValue = 0.0f;
  bool rawSampled = scale.sampleRawAverage(raw, 10, 3000UL);
  bool offsetSampled = rawSampled;
  if (offsetSampled) {
    offsetValue = raw - (float)scale.tareOffset();
  }

  print(F("raw_average="));
  print(raw, 2);
  print(F(" offset_value="));
  print(offsetValue, 2);
  print(F(" sampled="));
  print(offsetSampled);
  print(F(" offset="));
  print(scale.tareOffset());
  print(F(" scale_factor="));
  println(scale.scaleFactor(), 6);
}

void SerialConsole::runTareCommand(ScaleManager& scale) const {
  if (!scale.tareStable(6000UL)) {
    println(F("tare_error=scale_not_ready"));
    return;
  }

  println(F("tare_done"));
  print(F("SCALE_ZERO_OFFSET="));
  println(scale.tareOffset());
  println(F("Copy this value to HardwareConfig.h for permanent zero."));
}

void SerialConsole::runCalibrationCommand(ScaleManager& scale, float knownGrams) const {
  if (knownGrams <= 0.0f) {
    println(F("cal_error=missing_weight"));
    println(F("usage: cal <known grams>, example: cal 500"));
    return;
  }

  float raw = 0.0f;
  if (!scale.sampleRawAverage(raw, 25, 8000UL)) {
    println(F("cal_error=scale_not_ready"));
    return;
  }

  float offsetValue = raw - (float)scale.tareOffset();
  float newScale = offsetValue / knownGrams;
  scale.setScaleFactor(newScale);

  print(F("known_grams="));
  println(knownGrams, 2);
  print(F("offset_value="));
  println(offsetValue, 2);
  print(F("SCALE_CALIBRATION_FACTOR="));
  println(newScale, 6);
  print(F("check_grams="));
  println(scale.readUnits(10), 2);
  println(F("Copy SCALE_CALIBRATION_FACTOR to HardwareConfig.h."));
}

void SerialConsole::runTwoPointCalibrationCommand(ScaleManager& scale, const char* command) {
  if (strcmp(command, "cal2 reset") == 0 || strcmp(command, "cal2 clear") == 0) {
    _hasCalibrationPoint = false;
    println(F("cal2_reset_done"));
    return;
  }

  float knownGrams = parseCommandNumber(command);
  if (knownGrams < 0.0f) {
    println(F("cal2_error=negative_weight"));
    return;
  }

  float raw = 0.0f;
  if (!scale.sampleRawAverage(raw, 25, 8000UL)) {
    println(F("cal2_error=scale_not_ready"));
    return;
  }

  if (!_hasCalibrationPoint) {
    _hasCalibrationPoint = true;
    _calibrationPointGrams = knownGrams;
    _calibrationPointRaw = raw;
    print(F("cal2_point_1_grams="));
    println(_calibrationPointGrams, 2);
    print(F("cal2_point_1_raw="));
    println(_calibrationPointRaw, 2);
    println(F("Place second known weight and send cal2 <grams>."));
    return;
  }

  float gramsDelta = knownGrams - _calibrationPointGrams;
  float rawDelta = raw - _calibrationPointRaw;
  if (fabs(gramsDelta) < 1.0f || fabs(rawDelta) < 1.0f) {
    println(F("cal2_error=points_too_close"));
    println(F("Use two clearly different known weights, example 0g and 500g."));
    return;
  }

  float scaleFactor = rawDelta / gramsDelta;
  int32_t offset = (int32_t)(_calibrationPointRaw - (scaleFactor * _calibrationPointGrams));

  scale.setScaleFactor(scaleFactor);
  scale.setZeroOffset(offset);
  _hasCalibrationPoint = false;

  print(F("cal2_point_2_grams="));
  println(knownGrams, 2);
  print(F("cal2_point_2_raw="));
  println(raw, 2);
  print(F("SCALE_ZERO_OFFSET="));
  println(offset);
  print(F("SCALE_CALIBRATION_FACTOR="));
  println(scaleFactor, 6);
  print(F("check_grams="));
  println(scale.readUnits(10), 2);
  println(F("Copy both values to HardwareConfig.h."));
}

void SerialConsole::startGuidedCalibration(unsigned long nowMs) {
  resetGuidedCalibration();
  _calibrationGuideState = CalibrationGuideState::WaitWeight;
  _calibrationGuideAtMs = nowMs;
  println(F("calauto_start"));
  println(F("Guided calibration uses 3 known weights."));
  println(F("Enter weight 1/3 in grams, usually 0."));
  println(F("Type cancel to stop."));
}

bool SerialConsole::handleGuidedCalibrationInput(ScaleManager& scale, LedRing& ledRing, const char* command, unsigned long nowMs) {
  if (_calibrationGuideState == CalibrationGuideState::Idle) {
    return false;
  }

  if (strcmp(command, "cancel") == 0 || strcmp(command, "stop") == 0) {
    resetGuidedCalibration();
    ledRing.showCalibrationFeedback(CalibrationLedMode::Error, 0, CALIBRATION_GUIDE_POINTS, nowMs, 1200UL);
    println(F("calauto_cancelled"));
    return true;
  }

  if (_calibrationGuideState != CalibrationGuideState::WaitWeight) {
    println(F("calauto_busy"));
    return true;
  }

  char* end = NULL;
  float knownGrams = strtod(command, &end);
  while (end != NULL && *end == ' ') {
    end++;
  }

  if (end == command || (end != NULL && *end != '\0') || knownGrams < 0.0f) {
    println(F("calauto_error=enter_weight_grams_or_cancel"));
    return true;
  }

  _calibrationGuideGrams[_calibrationGuideIndex] = knownGrams;
  _calibrationGuideState = CalibrationGuideState::WaitSettle;
  _calibrationGuideAtMs = nowMs;
  ledRing.showCalibrationFeedback(
    CalibrationLedMode::Settle,
    _calibrationGuideIndex + 1,
    CALIBRATION_GUIDE_POINTS,
    nowMs,
    10000UL);
  print(F("calauto_place_weight="));
  println(knownGrams, 2);
  println(F("Sampling starts in 10 seconds."));
  return true;
}

void SerialConsole::updateGuidedCalibration(ScaleManager& scale, LedRing& ledRing, unsigned long nowMs) {
  if (_calibrationGuideState == CalibrationGuideState::Idle) {
    return;
  }

  if (_calibrationGuideState == CalibrationGuideState::WaitWeight) {
    ledRing.showCalibrationFeedback(
      CalibrationLedMode::Prompt,
      _calibrationGuideIndex + 1,
      CALIBRATION_GUIDE_POINTS,
      nowMs,
      500UL);
    return;
  }

  if (_calibrationGuideState == CalibrationGuideState::WaitSettle) {
    if (nowMs - _calibrationGuideAtMs < 10000UL) {
      return;
    }

    _calibrationGuideState = CalibrationGuideState::Sampling;
    ledRing.showCalibrationFeedback(
      CalibrationLedMode::Sampling,
      _calibrationGuideIndex + 1,
      CALIBRATION_GUIDE_POINTS,
      nowMs,
      9000UL);

    float raw = 0.0f;
    print(F("calauto_sampling_point="));
    println(_calibrationGuideIndex + 1);
    if (!scale.sampleRawAverage(raw, 45, 10000UL)) {
      println(F("calauto_error=scale_not_ready"));
      resetGuidedCalibration();
      ledRing.showCalibrationFeedback(CalibrationLedMode::Error, 0, CALIBRATION_GUIDE_POINTS, nowMs, 2500UL);
      return;
    }

    _calibrationGuideRaw[_calibrationGuideIndex] = raw;
    print(F("calauto_point_grams="));
    println(_calibrationGuideGrams[_calibrationGuideIndex], 2);
    print(F("calauto_point_raw="));
    println(raw, 2);

    _calibrationGuideIndex++;
    if (_calibrationGuideIndex >= CALIBRATION_GUIDE_POINTS) {
      finishGuidedCalibration(scale, ledRing, nowMs);
      return;
    }

    _calibrationGuideState = CalibrationGuideState::WaitWeight;
    print(F("Enter weight "));
    print(_calibrationGuideIndex + 1);
    print(F("/"));
    print(CALIBRATION_GUIDE_POINTS);
    println(F(" in grams."));
  }
}

void SerialConsole::resetGuidedCalibration() {
  _hasCalibrationPoint = false;
  _calibrationGuideState = CalibrationGuideState::Idle;
  _calibrationGuideIndex = 0;
  for (uint8_t i = 0; i < CALIBRATION_GUIDE_POINTS; i++) {
    _calibrationGuideGrams[i] = 0.0f;
    _calibrationGuideRaw[i] = 0.0f;
  }
}

void SerialConsole::finishGuidedCalibration(ScaleManager& scale, LedRing& ledRing, unsigned long nowMs) {
  float sumX = 0.0f;
  float sumY = 0.0f;
  float sumXX = 0.0f;
  float sumXY = 0.0f;

  for (uint8_t i = 0; i < CALIBRATION_GUIDE_POINTS; i++) {
    float grams = _calibrationGuideGrams[i];
    float raw = _calibrationGuideRaw[i];
    sumX += grams;
    sumY += raw;
    sumXX += grams * grams;
    sumXY += grams * raw;
  }

  float n = (float)CALIBRATION_GUIDE_POINTS;
  float denominator = n * sumXX - sumX * sumX;
  if (fabs(denominator) < 1.0f) {
    println(F("calauto_error=points_too_close"));
    println(F("Use three clearly different weights, example 0g, 250g, 500g."));
    resetGuidedCalibration();
    ledRing.showCalibrationFeedback(CalibrationLedMode::Error, 0, CALIBRATION_GUIDE_POINTS, nowMs, 2500UL);
    return;
  }

  float scaleFactor = (n * sumXY - sumX * sumY) / denominator;
  float offsetFloat = (sumY - scaleFactor * sumX) / n;
  int32_t offset = (int32_t)(offsetFloat + (offsetFloat >= 0.0f ? 0.5f : -0.5f));

  scale.setScaleFactor(scaleFactor);
  scale.setZeroOffset(offset);

  for (uint8_t i = 0; i < CALIBRATION_GUIDE_POINTS; i++) {
    print(F("calauto_used_point="));
    print(i + 1);
    print(F(" grams="));
    print(_calibrationGuideGrams[i], 2);
    print(F(" raw="));
    println(_calibrationGuideRaw[i], 2);
  }

  print(F("SCALE_ZERO_OFFSET="));
  println(offset);
  print(F("SCALE_CALIBRATION_FACTOR="));
  println(scaleFactor, 6);

  float checkGrams = 0.0f;
  if (scale.sampleUnitsAverage(checkGrams, 12, 3000UL)) {
    print(F("check_grams="));
    println(checkGrams, 2);
  }
  println(F("Copy both values to HardwareConfig.h."));
  resetGuidedCalibration();
  ledRing.showCalibrationFeedback(CalibrationLedMode::Success, CALIBRATION_GUIDE_POINTS, CALIBRATION_GUIDE_POINTS, nowMs, 5000UL);
}

TapCommand SerialConsole::update(
  const ScaleReading& reading,
  const HydrationStatus& status,
  ScaleManager& scale,
  LedRing& ledRing,
  VibrationMotor& vibration,
  unsigned long nowMs) {
  updateGuidedCalibration(scale, ledRing, nowMs);

  char command[48];
  if (!readLine(command, sizeof(command))) {
    return TapCommand::None;
  }

  normalizeCommand(command);

  if (handleGuidedCalibrationInput(scale, ledRing, command, nowMs)) {
    return TapCommand::None;
  }

  if (commandEquals(command, "h", "help") || strcmp(command, "?") == 0) {
    printHelp();
    return TapCommand::None;
  }
  if (commandEquals(command, "s", "status")) {
    printStatus(reading, status, TapCommand::None);
    return TapCommand::None;
  }
  if (commandEquals(command, "p", "progress")) {
    return TapCommand::ShowProgress;
  }
  if (commandEquals(command, "a", "pause")) {
    return TapCommand::TogglePause;
  }
  if (commandEquals(command, "m", "meeting")) {
    return TapCommand::ToggleMeeting;
  }
  if (commandEquals(command, "l", "light")) {
    return TapCommand::LightShow;
  }
  if (commandEquals(command, "n", "profile")) {
    return TapCommand::NextProfile;
  }
  if (commandEquals(command, "r", "reset")) {
    return TapCommand::ResetSession;
  }
  if (commandEquals(command, "e", "end")) {
    return TapCommand::EndDay;
  }
  if (commandEquals(command, "v", "vib")) {
    vibration.pulse(nowMs, 180);
    println(F("vibration_pulse"));
    return TapCommand::None;
  }
  if (commandEquals(command, "g", "grams")) {
    printScaleSnapshot(scale);
    return TapCommand::None;
  }
  if (strcmp(command, "raw") == 0) {
    printRawScaleSnapshot(scale);
    return TapCommand::None;
  }
  if (strcmp(command, "t") == 0 || strcmp(command, "tare") == 0 || strcmp(command, "zero") == 0) {
    runTareCommand(scale);
    return TapCommand::None;
  }
  if (strncmp(command, "cal ", 4) == 0) {
    runCalibrationCommand(scale, parseCommandNumber(command));
    return TapCommand::None;
  }
  if (strncmp(command, "cal2 ", 5) == 0) {
    runTwoPointCalibrationCommand(scale, command);
    return TapCommand::None;
  }
  if (strcmp(command, "calauto") == 0 || strcmp(command, "calibrate") == 0) {
    startGuidedCalibration(nowMs);
    return TapCommand::None;
  }

  print(F("unknown_command="));
  println(command);
  printHelp();
  return TapCommand::None;
}

void SerialConsole::printStatus(const ScaleReading& reading, const HydrationStatus& status, TapCommand command) const {
  print(F("grams="));
  print(reading.grams, 1);
  print(F(" instant_grams="));
  print(reading.instantGrams, 1);
  print(F(" stable="));
  print(reading.stable);
  print(F(" present="));
  print(reading.bottlePresent);
  print(F(" fresh="));
  print(reading.fresh);
  print(F(" age_ms="));
  print(reading.ageMs);
  print(F(" consumed_ml="));
  print(status.consumedMl);
  print(F(" target_ml="));
  print(status.targetMl);
  print(F(" recommended_ml="));
  print(status.recommendedSipMl);
  print(F(" balance_ml="));
  print(status.balanceMl);
  print(F(" progress_pct="));
  print((int)(status.progress * 100.0f));
  print(F(" profile="));
  print(status.profileName);
  print(F(" baseline="));
  print(status.baselineReady);
  print(F(" refill="));
  print(status.refillDetected);
  print(F(" bottle_change="));
  print(status.bottleChangeDetected);
  print(F(" command="));
  print(commandName(command));
  print(F(" state="));
  print(reminderStateName(status.state));
  print(F(" state_id="));
  println((int)status.state);
}

void SerialConsole::printDeducedEvents(
  const ScaleReading& reading,
  const HydrationStatus& status,
  TapCommand command) {
  _remoteEchoEnabled = true;

  if (!_eventStateInitialized) {
    _eventStateInitialized = true;
    _lastBottlePresent = reading.valid && reading.bottlePresent;
    _lastReadingValid = reading.valid;
    _lastReminderState = status.state;
    print(F("event=state_initial state="));
    println(reminderStateName(status.state));
  }

  if (reading.valid != _lastReadingValid) {
    println(reading.valid
      ? F("event=scale_recovered deduction=hx711_reading_valid")
      : F("event=scale_stale deduction=hx711_reading_missing_too_long"));
    _lastReadingValid = reading.valid;
  }

  bool bottlePresent = reading.valid && reading.bottlePresent;
  if (bottlePresent != _lastBottlePresent) {
    if (bottlePresent) {
      print(F("event=bottle_placed deduction=bottle_present grams="));
      println(reading.grams, 1);
    } else {
      println(F("event=bottle_removed deduction=bottle_missing"));
    }
    _lastBottlePresent = bottlePresent;
  }

  if (status.baselineCaptured) {
    print(F("event=baseline_captured deduction=stable_bottle_weight grams="));
    println(reading.grams, 1);
  }

  if (status.refillDetected) {
    print(F("event=refill deduction=weight_increased grams="));
    println(reading.grams, 1);
  }

  if (status.bottleChangeDetected) {
    print(F("event=bottle_change deduction=weight_drop_too_large_for_sip grams="));
    println(reading.grams, 1);
  }

  if (status.sipDetected) {
    print(F("event=sip deduction="));
    print(sipQualityName(status.lastSipQuality));
    print(F(" sip_ml="));
    print(status.lastSipMl);
    print(F(" reference_ml="));
    print(status.lastSipReferenceMl);
    print(F(" consumed_ml="));
    println(status.consumedMl);
  }

  if (command != TapCommand::None) {
    print(F("event=command deduction="));
    println(commandName(command));
  }

  if (status.state != _lastReminderState) {
    print(F("event=state_transition from="));
    print(reminderStateName(_lastReminderState));
    print(F(" to="));
    println(reminderStateName(status.state));
    _lastReminderState = status.state;
  }

  _remoteEchoEnabled = false;
}

void SerialConsole::printTapSequenceDetected(uint8_t tapCount) {
  _remoteEchoEnabled = true;
  print(F("event=tap_sequence count="));
  println(tapCount);
  _remoteEchoEnabled = false;
}

void SerialConsole::printTapPulseDetected(uint8_t tapCount) {
  _remoteEchoEnabled = true;
  print(F("event=tap_pulse count="));
  println(tapCount);
  _remoteEchoEnabled = false;
}

void SerialConsole::printPeriodicStatus(
  const ScaleReading& reading,
  const HydrationStatus& status,
  TapCommand command,
  unsigned long nowMs) {
  unsigned long debugInterval = testSerialEnabled() ? TEST_SERIAL_INTERVAL_MS : 2000UL;
  if (nowMs - _lastDebugMs < debugInterval) {
    return;
  }

  _lastDebugMs = nowMs;
  _remoteEchoEnabled = REMOTE_CONSOLE_FULL_LOGGING;
  printStatus(reading, status, command);
  _remoteEchoEnabled = false;
}

void SerialConsole::printCommandDetected(TapCommand command) {
  if (!testSerialEnabled() || command == TapCommand::None) {
    return;
  }

  _remoteEchoEnabled = REMOTE_CONSOLE_FULL_LOGGING;
  print(F("command_detected="));
  println(commandName(command));
  _remoteEchoEnabled = false;
}
