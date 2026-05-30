#include "Config.h"
#include "ScaleManager.h"
#include "HydrationTracker.h"
#include "LedRing.h"
#include "RemoteServices.h"
#include "SerialConsole.h"
#include "TapInput.h"
#include "VibrationMotor.h"

ScaleManager scaleManager;
HydrationTracker hydration;
LedRing ledRing;
RemoteServices remoteServices;
SerialConsole serialConsole;
TapInput tapInput;
VibrationMotor vibration;

void setup() {
  Serial.begin(115200);
  delay(100);

  ledRing.begin();
  vibration.begin(PIN_VIBRATION, VIBRATION_ENABLED);
  scaleManager.begin();
  hydration.begin(millis());
  remoteServices.begin();
  serialConsole.begin(scaleManager, &remoteServices);
}

void loop() {
  unsigned long nowMs = millis();
  remoteServices.update();
  ScaleReading reading = scaleManager.update(nowMs);
  HydrationStatus currentStatus = hydration.status();
  TapCommand command = tapInput.update(reading, nowMs);
  TapCommand serialCommand = serialConsole.update(reading, currentStatus, scaleManager, vibration, nowMs);
  if (serialCommand != TapCommand::None) {
    command = serialCommand;
  }
  hydration.handleCommand(command, nowMs);
  HydrationStatus status = hydration.update(reading, nowMs);

  ledRing.showCommandFeedback(command, status, nowMs);
  ledRing.render(status, nowMs);
  vibration.update(nowMs, status.overdue && status.vibrationAllowed);

  if (SerialConsole::testHardwareEnabled() && TEST_VIBRATE_ON_TAP && command != TapCommand::None) {
    vibration.pulse(nowMs, 70);
  }

  serialConsole.printCommandDetected(command);
  serialConsole.printPeriodicStatus(reading, status, command, nowMs);
}
