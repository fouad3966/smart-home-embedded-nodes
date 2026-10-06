#pragma once

#include <Arduino.h>

// Network credentials
extern const char* WIFI_SSID;
extern const char* WIFI_PASS;
extern const char* MQTT_SERVER;
extern const int MQTT_PORT;

// MQTT topics
extern const char* T_LAMP1;
extern const char* T_LAMP2;
extern const char* T_FAN1;
extern const char* T_FAN2;
extern const char* T_ALARM;
extern const char* T_CHPWD;
extern const char* T_CHPWD_STAT;
extern const char* T_STATUS;

// Pin definitions
constexpr uint8_t PIN_LAMP1 = D1;
constexpr uint8_t PIN_LAMP2 = D2;
constexpr uint8_t PIN_FAN1 = D5;
constexpr uint8_t PIN_FAN2 = D6;
constexpr uint8_t PIN_ALARM = D7;

// EEPROM
constexpr int EEPROM_SIZE = 16;
constexpr int PWD_ADDR = 0;
constexpr int PWD_LEN = 4;
