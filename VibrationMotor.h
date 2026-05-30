#pragma once

#include <Arduino.h>

class VibrationMotor {
public:
  void begin(uint8_t pin, bool enabled);
  void update(unsigned long nowMs, bool overdue);
  void pulse(unsigned long nowMs, uint16_t durationMs);

private:
  uint8_t _pin = 255;
  bool _enabled = false;
  bool _active = false;
  unsigned long _offAtMs = 0;
  unsigned long _lastReminderMs = 0;
};
