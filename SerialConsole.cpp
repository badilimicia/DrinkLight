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
  println(F("  cal2 reset    clear pending two-point calibration"));
  println(F("Example: tare, then put 500g, then cal 500"));
  println(F("Example: put 0g -> cal2 0, put 500g -> cal2 500"));
}

bool SerialConsole::readLine(char* out, uint8_t outSize) {
  if (_remote != NULL && _remote->readLine(out, outSize)) {
    _remoteEchoEnabled = true;
    return true;
  }

  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') {
      continue;
    }

    if (c == '\n') {
      _line[_lineLength] = '\0';
      strncpy(out, _line, outSize);
      out[outSize - 1] = '\0';
      _lineLength = 0;
      _remoteEchoEnabled = false;
      return out[0] != '\0';
    }

    if (_lineLength == 0 && strchr("hspamlnergvt?", c) != NULL) {
      out[0] = c;
      out[1] = '\0';
      _remoteEchoEnabled = false;
      return true;
    }

    if (_lineLength < sizeof(_line) - 1) {
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
  print(F("ready="));
  print(scale.isReady());
  print(F(" grams="));
  print(scale.readUnits(5), 2);
  print(F(" scale_factor="));
  print(scale.scaleFactor(), 6);
  print(F(" offset="));
  println(scale.tareOffset());
}

void SerialConsole::printRawScaleSnapshot(ScaleManager& scale) const {
  print(F("raw_average="));
  print(scale.readRawAverage(10), 2);
  print(F(" offset_value="));
  print(scale.readOffsetValue(10), 2);
  print(F(" offset="));
  print(scale.tareOffset());
  print(F(" scale_factor="));
  println(scale.scaleFactor(), 6);
}

void SerialConsole::runTareCommand(ScaleManager& scale) const {
  scale.tare();
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

  if (!scale.isReady()) {
    println(F("cal_error=scale_not_ready"));
    return;
  }

  float offsetValue = scale.readOffsetValue(20);
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

  if (!scale.isReady()) {
    println(F("cal2_error=scale_not_ready"));
    return;
  }

  float raw = scale.readRawAverage(20);
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

TapCommand SerialConsole::update(
  const ScaleReading& reading,
  const HydrationStatus& status,
  ScaleManager& scale,
  VibrationMotor& vibration,
  unsigned long nowMs) {
  char command[48];
  if (!readLine(command, sizeof(command))) {
    return TapCommand::None;
  }

  normalizeCommand(command);

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
  if (strcmp(command, "tare") == 0 || strcmp(command, "zero") == 0) {
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

  print(F("unknown_command="));
  println(command);
  printHelp();
  return TapCommand::None;
}

void SerialConsole::printStatus(const ScaleReading& reading, const HydrationStatus& status, TapCommand command) const {
  print(F("grams="));
  print(reading.grams, 1);
  print(F(" stable="));
  print(reading.stable);
  print(F(" present="));
  print(reading.bottlePresent);
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
  println((int)status.state);
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
