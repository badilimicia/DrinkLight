#pragma once

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "HydrationTracker.h"

class LedRing {
public:
  LedRing();
  void begin();
  void showCommandFeedback(TapCommand command, const HydrationStatus& status, unsigned long nowMs);
  void render(const HydrationStatus& status, unsigned long nowMs);

private:
  uint32_t color(uint8_t r, uint8_t g, uint8_t b) const;
  void clear();
  void showProgress(uint32_t baseColor, float progress, uint8_t brightness);
  void fillAll(uint32_t c);
  void sparkle(uint32_t c, unsigned long nowMs);
  void comet(uint32_t headColor, uint32_t tailColor, unsigned long nowMs, uint8_t tailLength);
  void vividProgress(uint32_t baseColor, float progress, unsigned long nowMs);
  void colorTest(unsigned long nowMs);
  bool renderCommandFeedback(const HydrationStatus& status, unsigned long nowMs);
  uint8_t styleBrightness(uint8_t calmValue) const;

  Adafruit_NeoPixel _pixels;
  TapCommand _feedbackCommand = TapCommand::None;
  unsigned long _feedbackStartedMs = 0;
  unsigned long _feedbackUntilMs = 0;
};
