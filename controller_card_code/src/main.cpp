#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>

#include "app_globals.h"
#include "config.h"
#include "network_manager.h"
#include "password_store.h"
#include "serial_handler.h"

void setup() {
  Serial.begin(9600);
  EEPROM.begin(EEPROM_SIZE);

  // Write LOW before pinMode to prevent boot glitch on ESP8266.
  digitalWrite(PIN_LAMP1, LOW);
  digitalWrite(PIN_LAMP2, LOW);
  digitalWrite(PIN_FAN1, LOW);
  digitalWrite(PIN_FAN2, LOW);
  digitalWrite(PIN_ALARM, LOW);

  pinMode(PIN_LAMP1, OUTPUT);
  pinMode(PIN_LAMP2, OUTPUT);
  pinMode(PIN_FAN1, OUTPUT);
  pinMode(PIN_FAN2, OUTPUT);
  pinMode(PIN_ALARM, OUTPUT);

  loadPassword();
  Serial.print("[PWD] Loaded password from EEPROM: ");
  Serial.println(appState.storedPassword);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("[WiFi] Connecting");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(WiFi.status() == WL_CONNECTED ? " OK" : " TIMEOUT");

  configureMQTT();
}

void loop() {
  ensureWiFi();
  ensureMQTT();
  mqtt.loop();
  handleSerial();
}