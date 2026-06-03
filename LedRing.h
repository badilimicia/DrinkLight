#pragma once

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "HydrationTracker.h"

enum class CalibrationLedMode {
  Off,
  Prompt,
  Settle,
  Sampling,
  Success,
  Error
};

class LedRing {
public:
  LedRing();
  void begin();
  void showCalibrationFeedback(CalibrationLedMode mode, uint8_t step, uint8_t totalSteps, unsigned long nowMs, unsigned long durationMs);
  void showTapSequenceFeedback(uint8_t tapCount, unsigned long nowMs);
  void showCommandFeedback(TapCommand command, const HydrationStatus& status, unsigned long nowMs);
  void render(const HydrationStatus& status, unsigned long nowMs);

private:
  uint32_t color(uint8_t r, uint8_t g, uint8_t b) const;
  void clear();
  void showProgress(uint32_t baseColor, float progress, uint8_t brightness);
  void fillAll(uint32_t c);
  void sparkle(uint32_t c, unsigned long nowMs);
  void comet(uint32_t headColor, uint32_t tailColor, unsigned long nowMs, uint8_t tailLength);
  void mirroredWave(uint32_t c, unsigned long nowMs, uint16_t speedMs);
  void rainbowSpin(unsigned long nowMs);
  void vividProgress(uint32_t baseColor, float progress, unsigned long nowMs);
  void colorTest(unsigned long nowMs);
  bool renderCommandFeedback(const HydrationStatus& status, unsigned long nowMs);
  bool renderCalibrationFeedback(unsigned long nowMs);
  bool renderTapSequenceFeedback(unsigned long nowMs);
  uint8_t styleBrightness(uint8_t calmValue) const;

  Adafruit_NeoPixel _pixels;
  TapCommand _feedbackCommand = TapCommand::None;
  CalibrationLedMode _calibrationMode = CalibrationLedMode::Off;
  uint8_t _calibrationStep = 0;
  uint8_t _calibrationTotalSteps = 0;
  uint8_t _tapFeedbackCount = 0;
  unsigned long _feedbackStartedMs = 0;
  unsigned long _feedbackUntilMs = 0;
  unsigned long _calibrationStartedMs = 0;
  unsigned long _calibrationUntilMs = 0;
  unsigned long _tapFeedbackStartedMs = 0;
  unsigned long _tapFeedbackUntilMs = 0;
};
