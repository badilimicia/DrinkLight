#include "RemoteServices.h"
#include <string.h>

void RemoteServices::begin() {
  if (!REMOTE_SERVICES_ENABLED) {
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long startedAt = millis();
  unsigned long timeoutMs = (unsigned long)WIFI_CONNECT_TIMEOUT_SEC * 1000UL;
  while (WiFi.status() != WL_CONNECTED && millis() - startedAt < timeoutMs) {
    delay(250);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("remote_wifi=failed"));
    return;
  }

  ArduinoOTA.setHostname(OTA_HOSTNAME);
  if (OTA_PASSWORD != NULL && OTA_PASSWORD[0] != '\0') {
    ArduinoOTA.setPassword(OTA_PASSWORD);
  }
  ArduinoOTA.begin();

  _server.begin();
  _server.setNoDelay(false);
  _started = true;
  printConnectionInfo();
}

void RemoteServices::update() {
  if (!_started) {
    return;
  }

  ArduinoOTA.handle();

  if ((!_client || !_client.connected()) && _server.hasClient()) {
    if (_client) {
      _client.stop();
    }
    _client = _server.available();
    _client.setNoDelay(false);
    _client.println(F("DrinkLight remote console"));
    _client.println(F("Type help for commands."));
  } else if (_server.hasClient()) {
    WiFiClient extra = _server.available();
    extra.println(F("Only one remote console client is supported."));
    extra.stop();
  }
}

bool RemoteServices::readLine(char* out, uint8_t outSize) {
  if (!_client || !_client.connected()) {
    return false;
  }

  while (_client.available()) {
    char c = (char)_client.read();
    if (c == '\r') {
      continue;
    }

    if (c == '\n') {
      _line[_lineLength] = '\0';
      strncpy(out, _line, outSize);
      out[outSize - 1] = '\0';
      _lineLength = 0;
      return out[0] != '\0';
    }

    if (_lineLength == 0 && strchr("hspamlnergvt?", c) != NULL) {
      out[0] = c;
      out[1] = '\0';
      return true;
    }

    if (_lineLength < sizeof(_line) - 1) {
      _line[_lineLength++] = c;
    }
  }

  return false;
}

bool RemoteServices::active() {
  return _started && _client.connected();
}

void RemoteServices::printConnectionInfo() {
  if (!_started) {
    return;
  }

  Serial.print(F("remote_wifi=connected ip="));
  Serial.print(WiFi.localIP());
  Serial.print(F(" ota="));
  Serial.print(OTA_HOSTNAME);
  Serial.print(F(" telnet_port="));
  Serial.println(REMOTE_CONSOLE_PORT);
}

void RemoteServices::println() {
  if (_client && _client.connected()) {
    _client.println();
  }
}
