#include "network.h"

namespace {

Network::MqttMessageHandler g_messageHandler = nullptr;
constexpr size_t kMaxPayloadLength = 255;

void mqttMessageRouter(char* topic, byte* payload, unsigned int length) {
  if (g_messageHandler == nullptr) {
    return;
  }

  char payloadBuffer[kMaxPayloadLength + 1];
  const unsigned int copyLength =
      length > kMaxPayloadLength ? kMaxPayloadLength : length;
  memcpy(payloadBuffer, payload, copyLength);
  payloadBuffer[copyLength] = '\0';

  g_messageHandler(topic, payloadBuffer);
}

}  // namespace

namespace Network {
//! setup wifi function


void setupWiFi(const char* ssid, const char* password) {
  delay(10);
  Serial.println("\nConnecting to WiFi...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}


//! setup mqtt server function

void setupMqttServer(PubSubClient& client, const char* server, uint16_t port) {
  client.setServer(server, port);
}

void setMqttMessageHandler(PubSubClient& client, MqttMessageHandler handler) {
  g_messageHandler = handler;
  client.setCallback(mqttMessageRouter);
}

bool subscribeToTopic(PubSubClient& client, const char* topic) {
  if (!client.connected()) {
    Serial.println("Cannot subscribe: MQTT client not connected");
    return false;
  }

  const bool subscribed = client.subscribe(topic);
  if (subscribed) {
    Serial.print("Subscribed to topic: ");
  } else {
    Serial.print("Failed to subscribe to topic: ");
  }
  Serial.println(topic);
  return subscribed;
}

void reconnectMqtt(PubSubClient& client,
                   const char* clientPrefix,
                   uint32_t retryDelayMs) {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    String clientId = String(clientPrefix) + "-" + String(ESP.getChipId(), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying...");
      delay(retryDelayMs);
    }
  }
}

//! configuring the json

bool publishJsonValue(PubSubClient& client, const char* topic, int value) {
  char payload[32];
  snprintf(payload, sizeof(payload), "{\"value\":%d}", value);
  return client.publish(topic, payload);
}

bool publishJsonValue(PubSubClient& client,
                      const char* topic,
                      float value,
                      uint8_t decimals) {
  char payload[48];
  snprintf(payload, sizeof(payload), "{\"value\":%.*f}", decimals, value);
  return client.publish(topic, payload);
}

bool publishJsonNull(PubSubClient& client, const char* topic) {
  return client.publish(topic, "{\"value\":null}");
}

}  // namespace Network
