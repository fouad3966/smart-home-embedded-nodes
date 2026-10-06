#pragma once

#include <Arduino.h>
#include <DHT.h>
#include <Wire.h>

namespace Sensors {

struct SensorReadings {
  float humidity;
  float temperatureC;
  int mq2Raw;
  int mq2Percent;
  int flameValue;
  bool mq2Ok;
  bool flameOk;
  bool dhtOk;
};

void initSensors(DHT& dht, uint8_t mq2Pin, uint8_t flamePin);
SensorReadings readSensors(DHT& dht, uint8_t mq2Pin, uint8_t flamePin);
void printSensorReadings(const SensorReadings& readings);

struct SlaveWeatherData {
  uint8_t waterPercent;
  uint8_t lightPercent;
  float   temperatureC;
  float   humidityPercent;
  bool    valid;
};

SlaveWeatherData requestSlaveWeather(uint8_t slaveAddr, uint8_t payloadSize);
void printSlaveWeather(const SlaveWeatherData& data);

}  // namespace Sensors
