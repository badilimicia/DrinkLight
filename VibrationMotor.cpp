#include "VibrationMotor.h"
#include "Config.h"

void VibrationMotor::begin(uint8_t pin, bool enabled) {
  _pin = pin;
  _enabled = enabled;
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, LOW);
}

void VibrationMotor::pulse(unsigned long nowMs, uint16_t durationMs) {
  if (!_enabled) {
    return;
  }

  _active = true;
  _offAtMs = nowMs + durationMs;
  digitalWrite(_pin, HIGH);
}

void VibrationMotor::update(unsigned long nowMs, bool overdue) {
  if (!_enabled) {
    return;
  }

  if (_active && (long)(nowMs - _offAtMs) >= 0) {
    _active = false;
    digitalWrite(_pin, LOW);
  }

  if (overdue && nowMs - _lastReminderMs >= (unsigned long)VIBRATION_REPEAT_SEC * 1000UL) {
    _lastReminderMs = nowMs;
    pulse(nowMs, VIBRATION_PULSE_MS);
  }
}
