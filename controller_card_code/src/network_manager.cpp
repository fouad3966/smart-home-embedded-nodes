#include "network_manager.h"

#include <ArduinoJson.h>

#include "app_globals.h"
#include "config.h"
#include "password_store.h"

namespace {

String extractSet(const String& raw) {
  StaticJsonDocument<128> doc;
  if (!deserializeJson(doc, raw)) {
    const char* val = doc["set"];
    if (val) return String(val);
  }
  return raw;
}

void callback(char* topic, byte* payload, unsigned int length) {
  String raw = "";
  raw.reserve(length);
  for (unsigned int i = 0; i < length; i++) raw += static_cast<char>(payload[i]);
  raw.trim();

  String t = String(topic);

  if (t == T_LAMP1 || t == T_LAMP2 || t == T_FAN1 || t == T_FAN2 || t == T_ALARM) {
    String cmd = extractSet(raw);
    bool on = (cmd == "on");

    if (t == T_LAMP1) {
      digitalWrite(PIN_LAMP1, on ? HIGH : LOW);
    } else if (t == T_LAMP2) {
      digitalWrite(PIN_LAMP2, on ? HIGH : LOW);
    } else if (t == T_FAN1) {
      digitalWrite(PIN_FAN1, on ? HIGH : LOW);
    } else if (t == T_FAN2) {
      digitalWrite(PIN_FAN2, on ? HIGH : LOW);
    } else if (t == T_ALARM) {
      appState.alarmState = on;
      digitalWrite(PIN_ALARM, on ? HIGH : LOW);
    }

    Serial.print("[MQTT] ");
    Serial.print(t);
    Serial.print(" -> ");
    Serial.println(on ? "ON" : "OFF");
    return;
  }

  if (t == T_CHPWD) {
    StaticJsonDocument<256> doc;
    if (deserializeJson(doc, raw)) {
      mqtt.publish(T_CHPWD_STAT, "FAIL:BAD_JSON");
      return;
    }

    String oldPwd = doc["oldPassword"] | "";
    String newPwd = doc["newPassword"] | "";
    oldPwd.trim();
    newPwd.trim();

    bool valid = (newPwd.length() == PWD_LEN);
    for (unsigned int i = 0; i < newPwd.length() && valid; i++) {
      if (!isDigit(newPwd[i])) valid = false;
    }

    if (oldPwd != appState.storedPassword) {
      mqtt.publish(T_CHPWD_STAT, "FAIL:WRONG_PASSWORD");
    } else if (!valid) {
      mqtt.publish(T_CHPWD_STAT, "FAIL:INVALID_NEW_PASSWORD");
    } else {
      appState.storedPassword = newPwd;
      savePassword(newPwd);
      mqtt.publish(T_CHPWD_STAT, "CHANGED");
      Serial.println("[PWD] Password updated.");
    }
  }
}

}  // namespace

void configureMQTT() {
  mqtt.setBufferSize(512);
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setCallback(callback);
}

void ensureWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  static unsigned long lastTry = 0;
  if (millis() - lastTry < 5000) return;
  lastTry = millis();

  Serial.println("[WiFi] Reconnecting...");
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

void ensureMQTT() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (mqtt.connected()) return;

  static unsigned long lastTry = 0;
  if (millis() - lastTry < 3000) return;
  lastTry = millis();

  String clientId = "smarthome-" + String(ESP.getChipId(), HEX);

  Serial.print("[MQTT] Connecting as ");
  Serial.print(clientId);
  Serial.print(" ... ");

  bool ok = mqtt.connect(
    clientId.c_str(),
    nullptr,
    nullptr,
    T_STATUS,
    0,
    true,
    "offline"
  );

  if (ok) {
    mqtt.publish(T_STATUS, "online", true);

    mqtt.subscribe(T_LAMP1, 1);
    mqtt.subscribe(T_LAMP2, 1);
    mqtt.subscribe(T_FAN1, 1);
    mqtt.subscribe(T_FAN2, 1);
    mqtt.subscribe(T_ALARM, 1);
    mqtt.subscribe(T_CHPWD, 0);

    Serial.println("OK");
    Serial.print("[WiFi] IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.print("FAILED rc=");
    Serial.println(mqtt.state());
  }
}
