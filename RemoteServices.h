#pragma once

#include <Arduino.h>
#include "Config.h"
#include <WiFi.h>
#include <ArduinoOTA.h>

class RemoteServices {
public:
  void begin();
  void update();
  bool readLine(char* out, uint8_t outSize);
  bool active();
  void printConnectionInfo();

  template <typename T>
  void print(const T& value) {
    if (_client && _client.connected()) {
      _client.print(value);
    }
  }

  template <typename T>
  void print(const T& value, int digits) {
    if (_client && _client.connected()) {
      _client.print(value, digits);
    }
  }

  template <typename T>
  void println(const T& value) {
    if (_client && _client.connected()) {
      _client.println(value);
    }
  }

  template <typename T>
  void println(const T& value, int digits) {
    if (_client && _client.connected()) {
      _client.println(value, digits);
    }
  }

  void println();

private:
  WiFiServer _server = WiFiServer(REMOTE_CONSOLE_PORT);
  WiFiClient _client;
  char _line[80] = {0};
  uint8_t _lineLength = 0;
  bool _started = false;
};
