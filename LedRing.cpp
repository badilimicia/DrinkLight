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

void LedRing::vividProgress(uint32_t baseColor, float progress, unsigned long nowMs) {
  showProgress(baseColor, progress, styleBrightness(LED_BRIGHTNESS));
  uint8_t head = (nowMs / 90UL) % LED_COUNT;
  _pixels.setPixelColor(head, color(180, 255, 255));
}

void LedRing::colorTest(unsigned long nowMs) {
  unsigned long elapsed = nowMs - _feedbackStartedMs;
  uint8_t phase = (elapsed / 650UL) % 8;
  uint32_t testColor = 0;

  switch (phase) {
    case 0:
      testColor = color(255, 0, 0);
      break;
    case 1:
      testColor = color(0, 255, 0);
      break;
    case 2:
      testColor = color(0, 0, 255);
      break;
    case 3:
      testColor = color(255, 255, 255);
      break;
    case 4:
      testColor = color(255, 180, 0);
      break;
    case 5:
      testColor = color(255, 0, 180);
      break;
    case 6:
      testColor = color(0, 220, 255);
      break;
    default:
      testColor = color(255, 80, 0);
      break;
  }

  _pixels.setBrightness(styleBrightness(LED_BRIGHTNESS));
  fillAll(testColor);
  _pixels.setPixelColor((nowMs / 90UL) % LED_COUNT, color(0, 0, 0));
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
      _pixels.setPixelColor(0, color(180, 90, 0));
      _pixels.setPixelColor(LED_COUNT / 2, color(180, 90, 0));
      break;
    }

    case ReminderState::Meeting:
      clear();
      if (status.reminderIntensity > 0.05f) {
        showProgress(color(0, 80, 120), status.reminderIntensity, 18);
      } else {
        _pixels.setBrightness(14);
        _pixels.setPixelColor(LED_COUNT / 4, color(0, 70, 110));
        _pixels.setPixelColor((LED_COUNT * 3) / 4, color(0, 70, 110));
      }
      break;

    case ReminderState::RefillDetected:
      if (LIGHT_STYLE == LightStyle::Vivid) {
        comet(color(0, 180, 255), color(0, 40, 160), nowMs, 6);
      } else {
        sparkle(color(0, 80, 220), nowMs);
      }
      break;

    case ReminderState::JustDrank:
      if (status.lastSipQuality == SipQuality::TooSmall) {
        showProgress(color(220, 120, 0), 0.35f, 55);
      } else if (status.lastSipQuality == SipQuality::Extra) {
        if (LIGHT_STYLE == LightStyle::Vivid) {
          comet(color(0, 220, 255), color(0, 90, 180), nowMs, 8);
        } else {
          sparkle(color(0, 140, 220), nowMs);
        }
      } else {
        if (LIGHT_STYLE == LightStyle::Vivid) {
          comet(color(60, 255, 120), color(0, 100, 60), nowMs, 6);
        } else {
          sparkle(color(0, 170, 80), nowMs);
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
        fillAll(LIGHT_STYLE == LightStyle::Vivid ? color(255, 35, 0) : color(220, 40, 0));
        if (LIGHT_STYLE == LightStyle::Vivid) {
          _pixels.setPixelColor((nowMs / 70UL) % LED_COUNT, color(255, 180, 0));
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
