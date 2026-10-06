#pragma once

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

struct AppState {
  String storedPassword = "1234";
  bool alarmState = false;
};

extern WiFiClient espClient;
extern PubSubClient mqtt;
extern AppState appState;
