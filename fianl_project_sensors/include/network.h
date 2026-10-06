#pragma once

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

namespace Network {

using MqttMessageHandler = void (*)(const char* topic, const char* payload);

void setupWiFi(const char* ssid, const char* password);
void setupMqttServer(PubSubClient& client, const char* server, uint16_t port);
void setMqttMessageHandler(PubSubClient& client, MqttMessageHandler handler);
bool subscribeToTopic(PubSubClient& client, const char* topic);
void reconnectMqtt(PubSubClient& client,
                   const char* clientPrefix,
                   uint32_t retryDelayMs);
bool publishJsonValue(PubSubClient& client, const char* topic, int value);
bool publishJsonValue(PubSubClient& client,
                      const char* topic,
                      float value,
                      uint8_t decimals = 2);
bool publishJsonNull(PubSubClient& client, const char* topic);

}  // namespace Network
