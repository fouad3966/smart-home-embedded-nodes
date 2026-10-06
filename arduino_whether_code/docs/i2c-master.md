# I2C master read guide

## Wiring
- UNO SDA = A4, SCL = A5
- Wemos D1 mini: SDA = D2 (GPIO4), SCL = D1 (GPIO5)
- Connect GND to GND on both boards
- Use a level shifter or 3.3V-safe pullups for SDA/SCL (UNO is 5V)

## I2C address
- Slave address: 0x12 (update `I2C_ADDR` in the UNO code if you change this)

## Payload format (6 bytes, little-endian)
- Byte 0: water sensor percent (0-100)
- Byte 1: LDR percent (0-100)
- Byte 2-3: temperature in Celsius x10 (int16)
- Byte 4-5: humidity percent x10 (int16)

## Example (ESP8266 / Wemos)
```cpp
#include <Wire.h>

const uint8_t I2C_ADDR = 0x12;

struct Payload {
  uint8_t water_pct;
  uint8_t ldr_pct;
  int16_t temp_c_x10;
  int16_t hum_x10;
};

Payload payload;

void setup() {
  Serial.begin(115200);
  Wire.begin(D2, D1);
}

void loop() {
  Wire.requestFrom(I2C_ADDR, (uint8_t)sizeof(Payload));
  if (Wire.available() == sizeof(Payload)) {
    Wire.readBytes((char *)&payload, sizeof(Payload));
    float tempC = payload.temp_c_x10 / 10.0f;
    float humPct = payload.hum_x10 / 10.0f;

    Serial.print("Water: ");
    Serial.print(payload.water_pct);
    Serial.print("%, LDR: ");
    Serial.print(payload.ldr_pct);
    Serial.print("%, Temp: ");
    Serial.print(tempC);
    Serial.print("C, Hum: ");
    Serial.print(humPct);
    Serial.println("%");
  }

  delay(2000);
}
```
