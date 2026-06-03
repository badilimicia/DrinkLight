#include "RemoteServices.h"
#include <errno.h>
#include <lwip/sockets.h>
#include <string.h>

namespace {
const char* WIFI_SSIDS[] = {
  WIFI_SSID,
  WIFI_FALLBACK_SSID
};

const char* WIFI_PASSWORDS[] = {
  WIFI_PASSWORD,
  WIFI_FALLBACK_PASSWORD
};

const uint8_t WIFI_NETWORK_COUNT = sizeof(WIFI_SSIDS) / sizeof(WIFI_SSIDS[0]);
}

void RemoteServices::begin() {
  if (!REMOTE_SERVICES_ENABLED) {
    return;
  }

  _started = true;
  configureWifi();
  connectNextNetwork();

  unsigned long startedAt = millis();
  unsigned long timeoutMs = (unsigned long)WIFI_CONNECT_TIMEOUT_SEC * 1000UL;
  while (WiFi.status() != WL_CONNECTED && millis() - startedAt < timeoutMs * WIFI_NETWORK_COUNT) {
    if (millis() - _lastReconnectAttemptMs >= timeoutMs) {
      connectNextNetwork();
    }
    delay(100);
  }

  if (WiFi.status() == WL_CONNECTED) {
    _wifiConnected = true;
    startNetworkServices();
    printConnectionInfo();
  } else {
    Serial.println(F("remote_wifi=failed local_mode=active"));
  }
}

void RemoteServices::configureWifi() {
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  WiFi.setHostname(OTA_HOSTNAME);
}

void RemoteServices::connectNextNetwork() {
  const char* ssid = WIFI_SSIDS[_nextNetworkIndex];
  const char* password = WIFI_PASSWORDS[_nextNetworkIndex];
  _nextNetworkIndex = (_nextNetworkIndex + 1) % WIFI_NETWORK_COUNT;
  _lastReconnectAttemptMs = millis();

  if (ssid == NULL || ssid[0] == '\0') {
    return;
  }

  Serial.print(F("remote_wifi_try ssid="));
  Serial.println(ssid);
  WiFi.disconnect();
  delay(50);
  WiFi.begin(ssid, password);
}

void RemoteServices::startNetworkServices() {
  if (_networkServicesStarted) {
    return;
  }

  ArduinoOTA.setHostname(OTA_HOSTNAME);
  if (OTA_PASSWORD != NULL && OTA_PASSWORD[0] != '\0') {
    ArduinoOTA.setPassword(OTA_PASSWORD);
  }

  ArduinoOTA.onStart([this]() {
    _otaActive = true;
    stopTelnetClient();
    Serial.println(F("ota=start"));
  });
  ArduinoOTA.onEnd([this]() {
    _otaActive = false;
    Serial.println(F("ota=end"));
  });
  ArduinoOTA.onError([this](ota_error_t error) {
    _otaActive = false;
    Serial.print(F("ota=error code="));
    Serial.println((int)error);
  });
  ArduinoOTA.begin();

  _server.begin();
  _server.setNoDelay(true);
  _networkServicesStarted = true;
}

void RemoteServices::stopNetworkServices() {
  stopTelnetClient();
  if (_networkServicesStarted) {
    ArduinoOTA.end();
    _server.end();
    _networkServicesStarted = false;
  }
  _otaActive = false;
}

void RemoteServices::update() {
  if (!_started) {
    return;
  }

  bool connected = WiFi.status() == WL_CONNECTED;
  if (!connected) {
    if (_wifiConnected) {
      Serial.println(F("remote_wifi=disconnected"));
      stopNetworkServices();
    }
    _wifiConnected = false;
    if (millis() - _lastReconnectAttemptMs >= RECONNECT_INTERVAL_MS) {
      connectNextNetwork();
    }
    return;
  }

  if (!_wifiConnected) {
    _wifiConnected = true;
    startNetworkServices();
    printConnectionInfo();
  }

  ArduinoOTA.handle();
  if (_otaActive) {
    return;
  }

  if ((!_client || !_client.connected()) && _server.hasClient()) {
    stopTelnetClient();
    _client = _server.available();
    _client.setNoDelay(true);
    println(F("DrinkLight remote console"));
    println(F("Type help for commands."));
  } else if (_server.hasClient()) {
    WiFiClient extra = _server.available();
    extra.println(F("Only one remote console client is supported."));
    extra.stop();
  }

  flushOutput();
  if (millis() - _lastHealthLogMs >= HEALTH_LOG_INTERVAL_MS) {
    _lastHealthLogMs = millis();
    printNetworkHealth();
  }
}

bool RemoteServices::readLine(char* out, uint8_t outSize) {
  if (!active()) {
    return false;
  }

  while (_client.available()) {
    uint8_t b = (uint8_t)_client.read();
    if (b == 255) {
      if (_client.available()) {
        _client.read();
      }
      if (_client.available()) {
        _client.read();
      }
      continue;
    }

    char c = (char)b;
    if (c == '\r' || c == '\n') {
      _line[_lineLength] = '\0';
      strncpy(out, _line, outSize);
      out[outSize - 1] = '\0';
      _lineLength = 0;
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

bool RemoteServices::active() {
  return _started && _wifiConnected && !_otaActive && _client.connected();
}

bool RemoteServices::otaActive() const {
  return _otaActive;
}

void RemoteServices::printConnectionInfo() {
  if (!_wifiConnected) {
    return;
  }

  Serial.print(F("remote_wifi=connected ssid="));
  Serial.print(WiFi.SSID());
  Serial.print(F(" ip="));
  Serial.print(WiFi.localIP());
  Serial.print(F(" gateway="));
  Serial.print(WiFi.gatewayIP());
  Serial.print(F(" rssi="));
  Serial.print(WiFi.RSSI());
  Serial.print(F(" ota="));
  Serial.print(OTA_HOSTNAME);
  Serial.print(F(" telnet_port="));
  Serial.println(REMOTE_CONSOLE_PORT);
}

void RemoteServices::printNetworkHealth() {
  Serial.print(F("remote_wifi=health ip="));
  Serial.print(WiFi.localIP());
  Serial.print(F(" gateway="));
  Serial.print(WiFi.gatewayIP());
  Serial.print(F(" rssi="));
  Serial.print(WiFi.RSSI());
  Serial.print(F(" telnet="));
  Serial.print(active());
  Serial.print(F(" ota_active="));
  Serial.println(_otaActive);
}

void RemoteServices::println() {
  static const char newline[] = "\r\n";
  if (active()) {
    enqueueOutput(newline, 2);
  }
}

void RemoteServices::enqueueOutput(const char* text, size_t length) {
  if (!active() || text == NULL || length == 0) {
    return;
  }

  if (length > OUTPUT_BUFFER_SIZE - _outputCount) {
    flushOutput();
  }

  if (length > OUTPUT_BUFFER_SIZE - _outputCount) {
    stopTelnetClient();
    return;
  }

  for (size_t i = 0; i < length; i++) {
    _output[_outputHead] = text[i];
    _outputHead = (_outputHead + 1) % OUTPUT_BUFFER_SIZE;
  }
  _outputCount += length;
}

void RemoteServices::flushOutput() {
  if (!active()) {
    clearOutput();
    return;
  }

  size_t budget = 256;
  while (_outputCount > 0 && budget > 0) {
    size_t contiguous = OUTPUT_BUFFER_SIZE - _outputTail;
    size_t toWrite = min(_outputCount, contiguous);
    toWrite = min(toWrite, budget);

    size_t written = writeSocketNonBlocking(&_output[_outputTail], toWrite);
    if (written == 0) {
      return;
    }

    _outputTail = (_outputTail + written) % OUTPUT_BUFFER_SIZE;
    _outputCount -= written;
    budget -= written;
  }
}

size_t RemoteServices::writeSocketNonBlocking(const char* text, size_t length) {
  if (!active() || text == NULL || length == 0) {
    return 0;
  }

  int written = send(_client.fd(), text, length, MSG_DONTWAIT);
  if (written > 0) {
    return (size_t)written;
  }

  if (written < 0 && errno != EWOULDBLOCK && errno != EAGAIN) {
    stopTelnetClient();
  }
  return 0;
}

void RemoteServices::stopTelnetClient() {
  clearOutput();
  _lineLength = 0;
  if (_client) {
    _client.stop();
  }
}

void RemoteServices::clearOutput() {
  _outputHead = 0;
  _outputTail = 0;
  _outputCount = 0;
}
