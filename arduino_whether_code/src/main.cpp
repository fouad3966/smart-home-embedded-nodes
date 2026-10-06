#include <Arduino.h>
#include <DHT.h>
#include <Wire.h>
#include <string.h>

namespace {
const uint8_t I2C_ADDR = 0x12;
const uint8_t WATER_PIN = A0;
const uint8_t LDR_PIN = A1;
const uint8_t DHT_PIN = 8;
const uint8_t DHT_TYPE = DHT11; // Change to DHT22 if needed.
const unsigned long SAMPLE_INTERVAL_MS = 2000;

struct SamplePayload {
  uint8_t water_pct;
  uint8_t ldr_pct;
  int16_t temp_c_x10;
  int16_t hum_x10;
};

DHT dht(DHT_PIN, DHT_TYPE);

SamplePayload lastSample = {0, 0, 0, 0};
volatile uint8_t payloadBytes[sizeof(SamplePayload)];
unsigned long lastSampleMs = 0;

uint8_t analogToPercent(int raw) {
  raw = constrain(raw, 0, 1023);
  return static_cast<uint8_t>(map(raw, 0, 1023, 0, 100));
}

int16_t toFixed10(float value) {
  if (value >= 0.0f) {
    return static_cast<int16_t>(value * 10.0f + 0.5f);
  }
  return static_cast<int16_t>(value * 10.0f - 0.5f);
}

void updatePayload(const SamplePayload &sample) {
  noInterrupts();
  memcpy((void *)payloadBytes, &sample, sizeof(SamplePayload));
  interrupts();
}

void onI2CRequest() {
  Wire.write((const uint8_t *)payloadBytes, sizeof(SamplePayload));
}

void sampleSensors() {
  SamplePayload sample = lastSample;

  sample.water_pct = analogToPercent(analogRead(WATER_PIN));
  sample.ldr_pct = analogToPercent(analogRead(LDR_PIN));

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (!isnan(humidity) && !isnan(temperature)) {
    sample.temp_c_x10 = toFixed10(temperature);
    sample.hum_x10 = toFixed10(humidity);
  }

  lastSample = sample;
  updatePayload(sample);
}
} // namespace

void setup() {
  Serial.begin(9600);
  dht.begin();

  Wire.begin(I2C_ADDR);
  Wire.onRequest(onI2CRequest);

  lastSampleMs = millis() - SAMPLE_INTERVAL_MS;
  updatePayload(lastSample);
}

void loop() {
  unsigned long now = millis();
  if (now - lastSampleMs >= SAMPLE_INTERVAL_MS) {
    lastSampleMs = now;
    sampleSensors();

    Serial.print("Water: ");
    Serial.print(lastSample.water_pct);
    Serial.print("%, LDR: ");
    Serial.print(lastSample.ldr_pct);
    Serial.print("%, Temp: ");
    Serial.print(lastSample.temp_c_x10 / 10.0f);
    Serial.print("C, Hum: ");
    Serial.print(lastSample.hum_x10 / 10.0f);
    Serial.println("%");
  }
}