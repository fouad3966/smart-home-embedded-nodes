#pragma once

#include <Arduino.h>
#include <DHT.h>
#include <Wire.h>

namespace AppConfig {

inline constexpr char WIFI_SSID[] = "Mnwr's M14";
inline constexpr char WIFI_PASSWORD[] = "mnwrameur20044";

inline constexpr char MQTT_SERVER[] = "10.228.191.110";
inline constexpr uint16_t MQTT_PORT = 1883;
inline constexpr char MQTT_CLIENT_PREFIX[] = "WeMosClient";

inline constexpr char GAS_TOPIC[] = "smartHome/devices/mq2/gas/state";
inline constexpr char TEMPERATURE_TOPIC[] = "smartHome/devices/dht11/temperature/state";
inline constexpr char HUMIDITY_TOPIC[] = "smartHome/devices/dht11/humidity/state";
inline constexpr char FIRE_TOPIC[] = "smartHome/devices/fireSensor/fire/state";

inline constexpr char WEATHER_WATER_TOPIC[]       = "smartHome/weather/water/state";
inline constexpr char WEATHER_LIGHT_TOPIC[]       = "smartHome/weather/light/state";
inline constexpr char WEATHER_TEMPERATURE_TOPIC[] = "smartHome/weather/temperature/state";
inline constexpr char WEATHER_HUMIDITY_TOPIC[]     = "smartHome/weather/humidity/state";

inline constexpr uint8_t I2C_SLAVE_ADDR = 0x12;
inline constexpr uint8_t I2C_PAYLOAD_SIZE = 6;


inline constexpr uint8_t DHT_PIN = D6;
inline constexpr uint8_t MQ2_PIN = A0;
inline constexpr uint8_t FLAME_PIN = D5;
inline constexpr uint8_t DHT_TYPE = DHT11;


inline constexpr uint32_t LOOP_DELAY_MS = 3000;
inline constexpr uint32_t MQTT_RETRY_DELAY_MS = 2000;

}  // namespace AppConfig
