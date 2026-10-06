#include "sensors.h"

namespace Sensors {

void initSensors(DHT& dht, uint8_t mq2Pin, uint8_t flamePin) {
  dht.begin();
  pinMode(mq2Pin, INPUT);
  pinMode(flamePin, INPUT);

  Serial.println("Sensors init complete");
}

SensorReadings readSensors(DHT& dht, uint8_t mq2Pin, uint8_t flamePin) {
  SensorReadings readings{};
  readings.humidity = dht.readHumidity();
  readings.temperatureC = dht.readTemperature();
  const int mq2Reading = analogRead(mq2Pin);
  const int mq2Clamped = constrain(mq2Reading, 0, 1023);
  const int mq2Percent = map(mq2Clamped, 0, 1023, 0, 100);
  const int flameReading = digitalRead(flamePin);
  readings.mq2Raw = mq2Reading;
  readings.mq2Percent = mq2Percent;
  readings.flameValue = flameReading;
  readings.mq2Ok = mq2Reading >= 0 && mq2Reading <= 1023;
  readings.flameOk = flameReading == LOW || flameReading == HIGH;
  readings.dhtOk = !(isnan(readings.humidity) || isnan(readings.temperatureC));
  return readings;
}

void printSensorReadings(const SensorReadings& readings) {
  Serial.println("------------------------------");

  if (!readings.dhtOk) {
    Serial.println("DHT read failed");
  } else {
    Serial.print("Temperature (C): ");
    Serial.println(readings.temperatureC);
    Serial.print("Humidity (%): ");
    Serial.println(readings.humidity);
  }

  Serial.print("MQ-2: ");
  if (readings.mq2Ok) {
    Serial.print(readings.mq2Percent);
    Serial.print("% (raw ");
    Serial.print(readings.mq2Raw);
    Serial.println(")");
  } else {
    Serial.println("null");
  }

  Serial.print("Flame: ");
  if (readings.flameOk) {
    Serial.println(readings.flameValue);
  } else {
    Serial.println("null");
  }
}

SlaveWeatherData requestSlaveWeather(uint8_t slaveAddr, uint8_t payloadSize) {
  SlaveWeatherData data{};
  data.valid = false;

  uint8_t received = Wire.requestFrom(slaveAddr, payloadSize);
  if (received != payloadSize) {
    Serial.print("I2C: expected ");
    Serial.print(payloadSize);
    Serial.print(" bytes, got ");
    Serial.println(received);
    // Drain any partial bytes
    while (Wire.available()) {
      Wire.read();
    }
    return data;
  }

  // Read the raw payload matching the slave's SamplePayload struct:
  //   uint8_t  water_pct    (1 byte)
  //   uint8_t  ldr_pct      (1 byte)
  //   int16_t  temp_c_x10   (2 bytes, little-endian)
  //   int16_t  hum_x10      (2 bytes, little-endian)
  uint8_t buf[6];
  for (uint8_t i = 0; i < payloadSize; ++i) {
    buf[i] = Wire.read();
  }

  data.waterPercent   = buf[0];
  data.lightPercent   = buf[1];

  int16_t tempX10 = static_cast<int16_t>(buf[2] | (buf[3] << 8));
  int16_t humX10  = static_cast<int16_t>(buf[4] | (buf[5] << 8));

  data.temperatureC   = tempX10 / 10.0f;
  data.humidityPercent = humX10 / 10.0f;
  data.valid = true;

  return data;
}

void printSlaveWeather(const SlaveWeatherData& data) {
  Serial.println("----- Slave Weather -----");
  if (!data.valid) {
    Serial.println("  (no valid data)");
    return;
  }
  Serial.print("  Water:       ");
  Serial.print(data.waterPercent);
  Serial.println("%");
  Serial.print("  Light:       ");
  Serial.print(data.lightPercent);
  Serial.println("%");
  Serial.print("  Temperature: ");
  Serial.print(data.temperatureC, 1);
  Serial.println(" C");
  Serial.print("  Humidity:    ");
  Serial.print(data.humidityPercent, 1);
  Serial.println("%");
}

}  // namespace Sensors
