#include <Arduino.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>

#include "app_config.h"
#include "network.h"
#include "sensors.h"

WiFiClient espClient;
PubSubClient mqttClient(espClient);
DHT dht(AppConfig::DHT_PIN, AppConfig::DHT_TYPE);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin();

  Network::setupWiFi(AppConfig::WIFI_SSID, AppConfig::WIFI_PASSWORD);
  Network::setupMqttServer(mqttClient, AppConfig::MQTT_SERVER, AppConfig::MQTT_PORT);

  Sensors::initSensors(dht, AppConfig::MQ2_PIN, AppConfig::FLAME_PIN);
}

void loop() {
  static uint32_t lastPublishMs = 0;

  if (!mqttClient.connected()) {
    Network::reconnectMqtt(mqttClient,
                           AppConfig::MQTT_CLIENT_PREFIX,
                           AppConfig::MQTT_RETRY_DELAY_MS);
  }

  mqttClient.loop();

  const uint32_t now = millis();
  if (lastPublishMs != 0 && now - lastPublishMs < AppConfig::LOOP_DELAY_MS) {
    return;
  }
  lastPublishMs = now;

  Sensors::SensorReadings readings =
      Sensors::readSensors(dht, AppConfig::MQ2_PIN, AppConfig::FLAME_PIN);

  if (readings.mq2Percent >= 2) {
    Network::publishJsonValue(mqttClient, AppConfig::GAS_TOPIC, readings.mq2Percent);
  } else {
    Network::publishJsonNull(mqttClient, AppConfig::GAS_TOPIC);
  }

  if (readings.flameOk) {
    Network::publishJsonValue(mqttClient, AppConfig::FIRE_TOPIC, readings.flameValue);
  } else {
    Network::publishJsonNull(mqttClient, AppConfig::FIRE_TOPIC);
  }

  if (readings.dhtOk) {
    Network::publishJsonValue(
        mqttClient, AppConfig::TEMPERATURE_TOPIC, readings.temperatureC, 2);
    Network::publishJsonValue(
        mqttClient, AppConfig::HUMIDITY_TOPIC, readings.humidity, 2);
  } else {
    Network::publishJsonNull(mqttClient, AppConfig::TEMPERATURE_TOPIC);
    Network::publishJsonNull(mqttClient, AppConfig::HUMIDITY_TOPIC);
  }

  // --- Slave weather data (I2C) ---
  Sensors::SlaveWeatherData weather =
      Sensors::requestSlaveWeather(AppConfig::I2C_SLAVE_ADDR,
                                   AppConfig::I2C_PAYLOAD_SIZE);
  Sensors::printSlaveWeather(weather);

  if (weather.valid) {
    Network::publishJsonValue(
        mqttClient, AppConfig::WEATHER_WATER_TOPIC,
        static_cast<int>(weather.waterPercent));
    Network::publishJsonValue(
        mqttClient, AppConfig::WEATHER_LIGHT_TOPIC,
        static_cast<int>(weather.lightPercent));
    Network::publishJsonValue(
        mqttClient, AppConfig::WEATHER_TEMPERATURE_TOPIC,
        weather.temperatureC, 1);
    Network::publishJsonValue(
        mqttClient, AppConfig::WEATHER_HUMIDITY_TOPIC,
        weather.humidityPercent, 1);
  } else {
    Network::publishJsonNull(mqttClient, AppConfig::WEATHER_WATER_TOPIC);
    Network::publishJsonNull(mqttClient, AppConfig::WEATHER_LIGHT_TOPIC);
    Network::publishJsonNull(mqttClient, AppConfig::WEATHER_TEMPERATURE_TOPIC);
    Network::publishJsonNull(mqttClient, AppConfig::WEATHER_HUMIDITY_TOPIC);
  }
}