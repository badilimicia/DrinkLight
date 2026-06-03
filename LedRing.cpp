#include "LedRing.h"
#include "Config.h"
#include <math.h>

LedRing::LedRing()
  : _pixels(LED_COUNT, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800) {
}

void LedRing::begin() {
  _pixels.begin();
  _pixels.setBrightness(styleBrightness(LED_BRIGHTNESS));
  clear();
  _pixels.show();
}

uint32_t LedRing::color(uint8_t r, uint8_t g, uint8_t b) const {
  return _pixels.Color(r, g, b);
}

void LedRing::clear() {
  for (uint16_t i = 0; i < LED_COUNT; i++) {
    _pixels.setPixelColor(i, 0);
  }
}

void LedRing::fillAll(uint32_t c) {
  for (uint16_t i = 0; i < LED_COUNT; i++) {
    _pixels.setPixelColor(i, c);
  }
}

void LedRing::showProgress(uint32_t baseColor, float progress, uint8_t brightness) {
  uint16_t lit = (uint16_t)(constrain(progress, 0.0f, 1.0f) * LED_COUNT + 0.5f);
  _pixels.setBrightness(brightness);
  clear();
  for (uint16_t i = 0; i < lit; i++) {
    _pixels.setPixelColor(i, baseColor);
  }
}

void LedRing::sparkle(uint32_t c, unsigned long nowMs) {
  clear();
  uint8_t head = (nowMs / 80UL) % LED_COUNT;
  for (uint8_t i = 0; i < 4; i++) {
    _pixels.setPixelColor((head + i * 4) % LED_COUNT, c);
  }
}

uint8_t LedRing::styleBrightness(uint8_t calmValue) const {
  if (LIGHT_STYLE == LightStyle::Vivid) {
    uint16_t vivid = (uint16_t)calmValue + 35;
    if (vivid > LED_VIVID_BRIGHTNESS) {
      vivid = LED_VIVID_BRIGHTNESS;
    }
    return (uint8_t)vivid;
  }
  return calmValue;
}

void LedRing::comet(uint32_t headColor, uint32_t tailColor, unsigned long nowMs, uint8_t tailLength) {
  clear();
  uint8_t head = (nowMs / 70UL) % LED_COUNT;
  for (uint8_t i = 0; i < tailLength; i++) {
    uint8_t index = (head + LED_COUNT - i) % LED_COUNT;
    _pixels.setPixelColor(index, i == 0 ? headColor : tailColor);
  }
}

void LedRing::mirroredWave(uint32_t c, unsigned long nowMs, uint16_t speedMs) {
  clear();
  uint8_t head = (nowMs / speedMs) % LED_COUNT;
  _pixels.setPixelColor(head, c);
  _pixels.setPixelColor((LED_COUNT - head) % LED_COUNT, c);
  _pixels.setPixelColor((head + LED_COUNT - 1) % LED_COUNT, color(70, 0, 110));
  _pixels.setPixelColor((LED_COUNT - head + 1) % LED_COUNT, color(70, 0, 110));
}

void LedRing::rainbowSpin(unsigned long nowMs) {
  for (uint16_t i = 0; i < LED_COUNT; i++) {
    uint16_t hue = (uint16_t)(((uint32_t)i * 65536UL / LED_COUNT + nowMs * 42UL) & 0xFFFF);
    _pixels.setPixelColor(i, _pixels.gamma32(_pixels.ColorHSV(hue, 255, 255)));
  }
}

void LedRing::vividProgress(uint32_t baseColor, float progress, unsigned long nowMs) {
  showProgress(baseColor, progress, styleBrightness(LED_BRIGHTNESS));
  uint8_t head = (nowMs / 90UL) % LED_COUNT;
  _pixels.setPixelColor(head, color(180, 255, 255));
}

void LedRing::colorTest(unsigned long nowMs) {
  unsigned long elapsed = nowMs - _feedbackStartedMs;
  _pixels.setBrightness(styleBrightness(LED_BRIGHTNESS));
  if (elapsed < 2200UL) {
    rainbowSpin(nowMs);
  } else if (elapsed < 4200UL) {
    mirroredWave(color(255, 0, 220), nowMs, 65);
  } else if (elapsed < 6100UL) {
    clear();
    uint8_t head = (nowMs / 75UL) % LED_COUNT;
    _pixels.setPixelColor(head, color(255, 255, 255));
    _pixels.setPixelColor((head + 2) % LED_COUNT, color(255, 30, 0));
    _pixels.setPixelColor((head + 4) % LED_COUNT, color(80, 255, 0));
    _pixels.setPixelColor((head + 6) % LED_COUNT, color(0, 80, 255));
  } else {
    uint8_t pulse = (uint8_t)(80 + 70 * (sin(nowMs / 130.0f) + 1.0f));
    _pixels.setBrightness(pulse);
    fillAll(color(255, 255, 255));
  }
}

void LedRing::showCalibrationFeedback(CalibrationLedMode mode, uint8_t step, uint8_t totalSteps, unsigned long nowMs, unsigned long durationMs) {
  _calibrationMode = mode;
  _calibrationStep = step;
  _calibrationTotalSteps = totalSteps == 0 ? 1 : totalSteps;
  _calibrationStartedMs = nowMs;
  _calibrationUntilMs = nowMs + durationMs;
}

void LedRing::showTapSequenceFeedback(uint8_t tapCount, unsigned long nowMs) {
  _tapFeedbackCount = tapCount;
  _tapFeedbackStartedMs = nowMs;
  _tapFeedbackUntilMs = nowMs + (unsigned long)tapCount * 360UL + 220UL;
}

void LedRing::showCommandFeedback(TapCommand command, const HydrationStatus& status, unsigned long nowMs) {
  if (command == TapCommand::None) {
    return;
  }

  _feedbackCommand = command;
  _feedbackStartedMs = nowMs;
  _feedbackUntilMs = nowMs + (command == TapCommand::LightShow ? 8000UL : 2500UL);
  if (command == TapCommand::ShowProgress && status.progress >= 0.98f) {
    _feedbackUntilMs = nowMs + 3500UL;
  }
}

bool LedRing::renderCalibrationFeedback(unsigned long nowMs) {
  if (_calibrationMode == CalibrationLedMode::Off || (long)(nowMs - _calibrationUntilMs) >= 0) {
    _calibrationMode = CalibrationLedMode::Off;
    return false;
  }

  unsigned long elapsed = nowMs - _calibrationStartedMs;
  float stepProgress = constrain((float)_calibrationStep / (float)_calibrationTotalSteps, 0.0f, 1.0f);

  switch (_calibrationMode) {
    case CalibrationLedMode::Prompt:
      showProgress(color(0, 90, 220), stepProgress, 45);
      _pixels.setPixelColor((nowMs / 250UL) % LED_COUNT, color(0, 180, 255));
      return true;

    case CalibrationLedMode::Settle: {
      float countdown = constrain((float)elapsed / (float)max(1UL, _calibrationUntilMs - _calibrationStartedMs), 0.0f, 1.0f);
      showProgress(color(220, 120, 0), countdown, 55);
      return true;
    }

    case CalibrationLedMode::Sampling:
      comet(color(255, 255, 255), color(70, 70, 70), nowMs, 5);
      return true;

    case CalibrationLedMode::Success:
      showProgress(color(0, 180, 80), 1.0f, 80);
      return true;

    case CalibrationLedMode::Error:
      if ((nowMs / 180UL) % 2 == 0) {
        fillAll(color(220, 0, 0));
      } else {
        clear();
      }
      return true;

    case CalibrationLedMode::Off:
      return false;
  }

  return false;
}

bool LedRing::renderTapSequenceFeedback(unsigned long nowMs) {
  if (_tapFeedbackCount == 0 || (long)(nowMs - _tapFeedbackUntilMs) >= 0) {
    _tapFeedbackCount = 0;
    return false;
  }

  unsigned long phase = (nowMs - _tapFeedbackStartedMs) / 180UL;
  clear();
  if (phase < (unsigned long)_tapFeedbackCount * 2UL && phase % 2UL == 0) {
    _pixels.setBrightness(95);
    fillAll(color(150, 0, 255));
  }
  return true;
}

bool LedRing::renderCommandFeedback(const HydrationStatus& status, unsigned long nowMs) {
  if (_feedbackCommand == TapCommand::None || (long)(nowMs - _feedbackUntilMs) >= 0) {
    _feedbackCommand = TapCommand::None;
    return false;
  }

  switch (_feedbackCommand) {
    case TapCommand::ShowProgress:
      vividProgress(color(0, 150, 90), status.progress, nowMs);
      return true;

    case TapCommand::TogglePause:
      clear();
      _pixels.setBrightness(60);
      _pixels.setPixelColor(0, color(220, 120, 0));
      _pixels.setPixelColor(LED_COUNT / 2, color(220, 120, 0));
      return true;

    case TapCommand::ToggleMeeting:
      showProgress(color(0, 80, 160), 0.5f, 45);
      return true;

    case TapCommand::LightShow:
      colorTest(nowMs);
      return true;

    case TapCommand::NextProfile: {
      float profileProgress = (float)(status.profileIndex + 1) / (float)DRINK_PROFILE_COUNT;
      vividProgress(color(120, 80, 255), profileProgress, nowMs);
      return true;
    }

    case TapCommand::ResetSession:
      comet(color(255, 255, 255), color(80, 80, 80), nowMs, 5);
      return true;

    case TapCommand::EndDay:
      showProgress(color(0, 150, 80), status.progress, 70);
      return true;

    case TapCommand::None:
      return false;
  }

  return false;
}

void LedRing::render(const HydrationStatus& status, unsigned long nowMs) {
  _pixels.setBrightness(styleBrightness(LED_BRIGHTNESS));

  if (renderCalibrationFeedback(nowMs)) {
    _pixels.show();
    return;
  }

  if (renderTapSequenceFeedback(nowMs)) {
    _pixels.show();
    return;
  }

  if (renderCommandFeedback(status, nowMs)) {
    _pixels.show();
    return;
  }

  switch (status.state) {
    case ReminderState::BottleMissing:
      // Soft blue locator while the puck is empty.
      clear();
      _pixels.setPixelColor((nowMs / 500UL) % LED_COUNT, color(0, 16, 48));
      break;

    case ReminderState::PausedManual:
    case ReminderState::PausedAuto: {
      clear();
      uint8_t phase = (uint8_t)((sin(nowMs / 900.0f) + 1.0f) * 18.0f);
      _pixels.setBrightness(20 + phase);
      _pixels.setPixelColor(0, color(255, 190, 0));
      _pixels.setPixelColor(LED_COUNT / 2, color(255, 190, 0));
      break;
    }

    case ReminderState::Meeting:
      clear();
      if (status.reminderIntensity > 0.05f) {
        showProgress(color(95, 40, 180), status.reminderIntensity, 18);
      } else {
        _pixels.setBrightness(14);
        _pixels.setPixelColor(LED_COUNT / 4, color(95, 40, 180));
        _pixels.setPixelColor((LED_COUNT * 3) / 4, color(95, 40, 180));
      }
      break;

    case ReminderState::RefillDetected:
      mirroredWave(color(210, 0, 255), nowMs, 85);
      break;

    case ReminderState::JustDrank:
      if (status.lastSipQuality == SipQuality::TooSmall) {
        float sipRatio = status.lastSipReferenceMl > 0
          ? (float)status.lastSipMl / (float)status.lastSipReferenceMl
          : 0.0f;
        showProgress(color(220, 120, 0), constrain(sipRatio, 0.12f, 0.85f), 55);
      } else if (status.lastSipQuality == SipQuality::Extra) {
        if (LIGHT_STYLE == LightStyle::Vivid) {
          comet(color(190, 255, 0), color(40, 120, 0), nowMs, 8);
        } else {
          sparkle(color(190, 255, 0), nowMs);
        }
      } else {
        if (LIGHT_STYLE == LightStyle::Vivid) {
          mirroredWave(color(0, 255, 70), nowMs, 100);
        } else {
          sparkle(color(0, 255, 70), nowMs);
        }
      }
      break;

    case ReminderState::Quiet:
      clear();
      break;

    case ReminderState::Ramp: {
      uint8_t brightness = (uint8_t)(10 + status.reminderIntensity * 70.0f);
      if (LIGHT_STYLE == LightStyle::Vivid) {
        vividProgress(color(0, 80, 180), status.reminderIntensity, nowMs);
      } else {
        showProgress(color(0, 80, 180), status.reminderIntensity, brightness);
      }
      break;
    }

    case ReminderState::Due: {
      uint8_t pulse = (uint8_t)(45 + 35 * (sin(nowMs / 350.0f) + 1.0f));
      if (LIGHT_STYLE == LightStyle::Vivid) {
        vividProgress(color(0, 160, 120), 1.0f, nowMs);
      } else {
        showProgress(color(0, 160, 120), 1.0f, pulse);
      }
      break;
    }

    case ReminderState::Overdue:
      if ((nowMs / 250UL) % 2 == 0) {
        fillAll(LIGHT_STYLE == LightStyle::Vivid ? color(255, 0, 40) : color(220, 0, 30));
        if (LIGHT_STYLE == LightStyle::Vivid) {
          _pixels.setPixelColor((nowMs / 70UL) % LED_COUNT, color(255, 255, 255));
        }
      } else {
        clear();
      }
      break;

    case ReminderState::Complete:
      if (LIGHT_STYLE == LightStyle::Vivid) {
        comet(color(120, 255, 120), color(0, 140, 60), nowMs, 10);
      } else {
        showProgress(color(0, 180, 80), 1.0f, LED_BRIGHTNESS);
      }
      break;

    case ReminderState::DayComplete:
      if (LIGHT_STYLE == LightStyle::Vivid) {
        vividProgress(color(0, 120, 80), status.progress, nowMs);
      } else {
        showProgress(color(0, 120, 80), status.progress, 55);
      }
      break;
  }

  _pixels.show();
}
