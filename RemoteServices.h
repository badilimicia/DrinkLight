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
  bool otaActive() const;
  void printConnectionInfo();

  template <typename T>
  void print(const T& value) {
    if (active()) {
      String text(value);
      enqueueOutput(text.c_str(), text.length());
    }
  }

  template <typename T>
  void print(const T& value, int digits) {
    if (active()) {
      String text(value, digits);
      enqueueOutput(text.c_str(), text.length());
    }
  }

  template <typename T>
  void println(const T& value) {
    print(value);
    println();
  }

  template <typename T>
  void println(const T& value, int digits) {
    print(value, digits);
    println();
  }

  void println();

private:
  void configureWifi();
  void connectNextNetwork();
  void startNetworkServices();
  void stopNetworkServices();
  void stopTelnetClient();
  void printNetworkHealth();
  void enqueueOutput(const char* text, size_t length);
  void flushOutput();
  size_t writeSocketNonBlocking(const char* text, size_t length);
  void clearOutput();

  static const size_t OUTPUT_BUFFER_SIZE = 4096;
  static const unsigned long RECONNECT_INTERVAL_MS = 10000UL;
  static const unsigned long HEALTH_LOG_INTERVAL_MS = 30000UL;

  WiFiServer _server = WiFiServer(REMOTE_CONSOLE_PORT);
  WiFiClient _client;
  char _line[80] = {0};
  char _output[OUTPUT_BUFFER_SIZE] = {0};
  uint8_t _lineLength = 0;
  size_t _outputHead = 0;
  size_t _outputTail = 0;
  size_t _outputCount = 0;
  bool _started = false;
  bool _networkServicesStarted = false;
  bool _wifiConnected = false;
  bool _otaActive = false;
  uint8_t _nextNetworkIndex = 0;
  unsigned long _lastReconnectAttemptMs = 0;
  unsigned long _lastHealthLogMs = 0;
};
