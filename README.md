# Smart Home Embedded Systems: Technical Report

**Academic Documentation — Embedded Hardware Nodes & Communication Architecture**

---

## Table of Contents

1. [Abstract](#1-abstract)
2. [Introduction](#2-introduction)
3. [System Architecture Overview](#3-system-architecture-overview)
4. [Hardware Node Specifications](#4-hardware-node-specifications)
5. [Communication Protocols](#5-communication-protocols)
6. [Node 1: Controller Card (ESP8266)](#6-node-1-controller-card-esp8266)
7. [Node 2: Door Card (Arduino Uno)](#7-node-2-door-card-arduino-uno)
8. [Node 3: Sensor Node (ESP8266)](#8-node-3-sensor-node-esp8266)
9. [Node 4: Weather Slave (Arduino Uno)](#9-node-4-weather-slave-arduino-uno)
10. [Node 5: ESP32 RFID Reader](#10-node-5-esp32-rfid-reader)
11. [Node 6: RFID Simulator (Flutter App)](#11-node-6-rfid-simulator-flutter-app)
12. [MQTT Topic Hierarchy](#12-mqtt-topic-hierarchy)
13. [Backend Integration](#13-backend-integration)
14. [Data Flow Analysis](#14-data-flow-analysis)
15. [Security Considerations](#15-security-considerations)
16. [Conclusion](#16-conclusion)

---

## 1. Abstract

This report presents a comprehensive technical analysis of the embedded systems comprising the hardware layer of a Smart Home automation platform. The system employs a distributed architecture consisting of six distinct nodes communicating via three protocols: **MQTT** (Message Queuing Telemetry Transport) over Wi-Fi, **I2C** (Inter-Integrated Circuit) for inter-board sensor aggregation, and **UART Serial** for local co-processor password verification. Each node is documented with its microcontroller platform, peripheral interfaces, firmware logic, and role within the broader IoT ecosystem. The RFID Simulator, a Flutter-based mobile application, replicates the functionality of the ESP32 RFID hardware node, serving as a software-defined alternative for NFC tag scanning and MQTT publication.

---

## 2. Introduction

### 2.1 Context

Home automation systems require a heterogeneous network of embedded devices to bridge the physical environment with digital control planes. This project implements such a network using commercially available microcontroller boards (ESP8266 WeMos D1, Arduino Uno, ESP32) programmed via the Arduino framework and managed through the PlatformIO build system.

### 2.2 Objectives

- Control household actuators (lamps, fans, alarm, door lock) via MQTT commands
- Collect environmental telemetry (temperature, humidity, gas, flame, water level, light) and publish to MQTT
- Implement RFID-based access control with both hardware (MFRC522) and software (NFC mobile) readers
- Provide a password-based door lock mechanism with EEPROM-persistent credential storage
- Aggregate data from I2C slave sensors into a single MQTT-publishing master node

### 2.3 Development Environment

| Tool | Version / Detail |
|------|-----------------|
| PlatformIO | Build system for all C++ firmware |
| Arduino Framework | Runtime for ESP8266, ESP32, ATmega328P |
| Flutter SDK | >= 3.11.5 (RFID Simulator) |
| MQTT Broker | Mosquitto at `10.228.191.110:1883` |

---

## 3. System Architecture Overview

The system follows a **hub-and-spoke** topology where all Wi-Fi-capable nodes communicate through a central MQTT broker. Non-networked nodes (Arduino Uno boards) interface with networked nodes via wired protocols (Serial UART, I2C).

```
+------------------------------------------------------------------+
|                        MQTT Broker                                |
|                    (Mosquitto :1883)                              |
+------+----------+----------+----------+--------------------------+
       |          |          |          |
  +----v---+ +---v----+ +---v----+ +---v------------------+
  |ESP8266 | |ESP8266 | | ESP32  | | Flutter App           |
  |Control | |Sensors | | RFID   | | RFID Simulator        |
  |Card    | |Node    | | Reader | | (mobile NFC)          |
  +---+----+ +---+----+ +--------+ +-----------------------+
      |UART      |I2C
  +---v----+ +---v----+
  |Arduino | |Arduino |
  |Uno     | |Uno     |
  |Door    | |Weather |
  |Card    | |Slave   |
  +--------+ +--------+
```

### 3.1 Communication Protocol Summary

| Link | Protocol | Speed | Direction |
|------|----------|-------|-----------|
| Controller <-> Broker | MQTT/TCP | Wi-Fi | Bidirectional (subscribe + publish) |
| Sensor Node <-> Broker | MQTT/TCP | Wi-Fi | Publish only |
| ESP32 RFID <-> Broker | MQTT/TCP | Wi-Fi | Publish only |
| RFID Simulator <-> Broker | MQTT/TCP | Wi-Fi | Publish only |
| Controller <-> Door Card | UART Serial | 9600 baud | Bidirectional |
| Sensor Node <-> Weather Slave | I2C | 100 kHz | Master-read |
| NestJS Backend <-> Broker | MQTT/TCP | LAN | Bidirectional |

---

## 4. Hardware Node Specifications

| Node | MCU | Board | Framework | Key Peripherals |
|------|-----|-------|-----------|----------------|
| Controller Card | ESP8266 | WeMos D1 | Arduino | 5 GPIO outputs (lamps, fans, alarm) |
| Door Card | ATmega328P | Arduino Uno | Arduino | 4x3 Keypad, 16x2 I2C LCD, Solenoid lock, Buzzer |
| Sensor Node | ESP8266 | WeMos D1 | Arduino | DHT11, MQ-2, Flame sensor, I2C master |
| Weather Slave | ATmega328P | Arduino Uno | Arduino | DHT11, Water sensor, LDR, I2C slave |
| ESP32 RFID | ESP32 | DevKit | Arduino | MFRC522 (SPI), Wi-Fi |
| RFID Simulator | N/A | Android/iOS | Flutter | NFC hardware (phone), Wi-Fi |

---

## 5. Communication Protocols

### 5.1 MQTT (Message Queuing Telemetry Transport)

MQTT is the primary backbone protocol. All Wi-Fi-capable nodes connect to a single Mosquitto broker. The system uses:
- **QoS 0** for sensor telemetry (fire-and-forget)
- **QoS 1** for actuator commands (at-least-once delivery)
- **Last Will and Testament (LWT)** on the Controller Card to publish `"offline"` to `smartHome/status` on unexpected disconnection
- **Retained messages** for status topics so new subscribers receive current state

The PubSubClient library (v2.8) handles MQTT on ESP8266/ESP32. The Flutter RFID Simulator uses the `mqtt_client` Dart package (v10.8.0).

### 5.2 I2C (Inter-Integrated Circuit)

Used between the **Sensor Node** (master, ESP8266) and the **Weather Slave** (slave, Arduino Uno) at address `0x12`. The slave responds to master read-requests with a 6-byte binary payload:

| Byte Offset | Field | Type | Range |
|-------------|-------|------|-------|
| 0 | `water_pct` | uint8 | 0-100 |
| 1 | `ldr_pct` | uint8 | 0-100 |
| 2-3 | `temp_c_x10` | int16 LE | Temperature x 10 |
| 4-5 | `hum_x10` | int16 LE | Humidity x 10 |

> **Note:** A level shifter is required between the 5V Uno and 3.3V ESP8266 I2C lines.

### 5.3 UART Serial

Used between the **Controller Card** (ESP8266) and the **Door Card** (Arduino Uno) at **9600 baud**. The protocol is a simple text-based request/response:

```
Door Card -> Controller:  "REQ:<4-digit-pin>\n"
Controller -> Door Card:  "OK\n"  or  "FAIL\n"
```

The Controller validates the PIN against its EEPROM-stored password and responds within 500ms (timeout). The Door Card enforces a 3-second timeout on the serial response.

---

## 6. Node 1: Controller Card (ESP8266)

### 6.1 Purpose

The Controller Card is the **central actuator hub**. It subscribes to MQTT topics for lamps, fans, alarm, and door password management, and drives GPIO outputs to control physical relays/actuators. It also acts as the **serial bridge** for Door Card password verification.

### 6.2 Pin Mapping

| Function | GPIO | ESP8266 Label |
|----------|------|---------------|
| Lamp 1 | D1 | GPIO5 |
| Lamp 2 | D2 | GPIO4 |
| Fan 1 | D5 | GPIO14 |
| Fan 2 | D6 | GPIO12 |
| Alarm | D7 | GPIO13 |
| Serial TX/RX | N/A | UART0 (to Door Card) |

### 6.3 Firmware Modules

| File | Responsibility |
|------|---------------|
| `main.cpp` | Initialization, Wi-Fi connection, main loop |
| `config.cpp` / `config.h` | Wi-Fi credentials, MQTT broker address, topic strings, pin constants |
| `app_globals.cpp` / `app_globals.h` | Global PubSubClient, WiFiClient, AppState struct |
| `network_manager.cpp` | MQTT callback, configureMQTT(), ensureWiFi(), ensureMQTT() |
| `password_store.cpp` | EEPROM read/write for 4-digit door password |
| `serial_handler.cpp` | UART protocol handler for Door Card requests |

### 6.4 MQTT Callback Logic

When a message arrives on a subscribed topic, the callback:

1. **Actuator topics** (`lamp1/set`, `lamp2/set`, `fan1/set`, `fan2/set`, `alarm/set`): Extracts the `"set"` field from JSON (or uses raw payload), and drives the corresponding GPIO HIGH or LOW.
2. **Password change topic** (`door/changePassword`): Parses JSON with `oldPassword` and `newPassword` fields, validates the old password against stored state, checks the new password is exactly 4 digits, and responds on the status topic with `CHANGED`, `FAIL:WRONG_PASSWORD`, or `FAIL:INVALID_NEW_PASSWORD`.

### 6.5 Reconnection Strategy

- **Wi-Fi**: Non-blocking retry every 5 seconds via `ensureWiFi()`
- **MQTT**: Non-blocking retry every 3 seconds via `ensureMQTT()`, with LWT set to `"offline"` on `smartHome/status`

---

## 7. Node 2: Door Card (Arduino Uno)

### 7.1 Purpose

The Door Card provides the **physical user interface** for door access. Users enter a 4-digit PIN on a matrix keypad, which is sent over Serial UART to the Controller Card for verification.

### 7.2 Hardware Interface

| Component | Connection |
|-----------|-----------|
| 4x3 Matrix Keypad | Rows: pins 9,8,7,6 / Cols: pins 5,4,3 |
| 16x2 LCD (I2C) | Address `0x27`, SDA/SCL |
| Solenoid Lock | Pin 12 (via relay/MOSFET) |
| Buzzer | Pin 2 |

### 7.3 Operation Sequence

```
+----------+     +--------------+     +--------------+
|  User    |     |  Door Card   |     | Controller   |
|          |     |  (Uno)       |     | (ESP8266)    |
+----+-----+     +------+-------+     +------+-------+
     |  Press keys      |                    |
     | --------------->>|                    |
     |                  |  "REQ:1234\n"      |
     |                  | ---------------->> |
     |                  |                    | Compare with
     |                  |                    | EEPROM password
     |                  |  "OK\n"            |
     |                  | <<---------------- |
     |  LCD: "ACCESS    |                    |
     |  GRANTED"        |                    |
     | <<---------------|                    |
     |                  | Activate solenoid  |
     |                  | for 3 seconds      |
```

### 7.4 Key Behaviors

- `*` key: Clears current input
- `#` key: Submits the 4-digit PIN
- Input capped at 4 characters; LCD shows `*` for each digit
- **Access Granted**: LCD displays message, solenoid lock opens for 3s, single 1kHz beep
- **Access Denied**: LCD displays message, three 400Hz warning beeps
- **Timeout**: If no serial response within 3s, LCD shows "NO RESPONSE"

---

## 8. Node 3: Sensor Node (ESP8266)

### 8.1 Purpose

The Sensor Node is the **telemetry aggregator**. It reads local sensors (DHT11, MQ-2, flame) and remote sensors (via I2C from the Weather Slave), then publishes all readings to MQTT.

### 8.2 Sensor Suite

| Sensor | Type | Pin | Measurement |
|--------|------|-----|-------------|
| DHT11 | Digital | D6 | Temperature (C), Humidity (%) |
| MQ-2 | Analog | A0 | Gas concentration (0-100%) |
| Flame Sensor | Digital | D5 | Fire detection (HIGH/LOW) |
| I2C Slave | Bus | SDA/SCL | Water, Light, Temp, Humidity (from Weather Slave) |

### 8.3 Publishing Logic

The firmware publishes every **3 seconds** (`LOOP_DELAY_MS = 3000`). Each sensor value is wrapped in a JSON envelope:

```json
{"value": 25.50}
{"value": null}
```

**Gas threshold**: MQ-2 readings below 2% are published as `null` to filter noise.

### 8.4 I2C Master Read

The Sensor Node requests 6 bytes from the Weather Slave at address `0x12`. The binary payload is decoded per the format in Section 5.2 and published to four weather-specific MQTT topics.

---

## 9. Node 4: Weather Slave (Arduino Uno)

### 9.1 Purpose

The Weather Slave is a **peripheral sensor board** that samples environmental data and exposes it via I2C to the Sensor Node master. It has **no network connectivity** — all data egress is through the I2C bus.

### 9.2 Sensor Readings

| Sensor | Pin | Output |
|--------|-----|--------|
| Water Level | A0 | Analog, 0-100% |
| LDR (Light) | A1 | Analog, 0-100% |
| DHT11 | D8 | Temperature (C), Humidity (%) |

### 9.3 I2C Slave Implementation

The slave registers an `onRequest` ISR callback that writes the 6-byte `SamplePayload` struct to the I2C bus when the master requests data. Sensor sampling occurs every 2 seconds in the main loop, and the payload is updated atomically using `noInterrupts()`/`interrupts()` guards to prevent data tearing during ISR access.

```cpp
struct SamplePayload {
  uint8_t water_pct;     // 1 byte
  uint8_t ldr_pct;       // 1 byte
  int16_t temp_c_x10;    // 2 bytes, little-endian
  int16_t hum_x10;       // 2 bytes, little-endian
};
```

### 9.4 Fixed-Point Encoding

Temperature and humidity are encoded as **fixed-point integers** (value x 10) to avoid floating-point transmission over I2C. The master divides by 10.0 to recover the original float value.

---

## 10. Node 5: ESP32 RFID Reader

### 10.1 Purpose

The ESP32 RFID Reader scans **MIFARE RFID cards** using the MFRC522 module and publishes the card UID to MQTT for access control verification by the backend.

### 10.2 Hardware

| Component | Connection |
|-----------|-----------|
| MFRC522 RFID Module | SPI: SS=GPIO5, RST=GPIO21, MOSI/MISO/SCK=default SPI |
| ESP32 DevKit | Built-in Wi-Fi |

### 10.3 Operation

1. Initialize SPI bus and MFRC522 reader
2. Connect to Wi-Fi and MQTT broker
3. In the main loop, poll for a new card via `PICC_IsNewCardPresent()` and `PICC_ReadCardSerial()`
4. Read the UID bytes and convert to uppercase hexadecimal string (e.g., `"A3B2C1D0"`)
5. Construct JSON payload and publish to MQTT:

```json
{"cardTag": "A3B2C1D0"}
```

**Topic**: `smartHome/devices/door/testCard`

6. Halt the card (`PICC_HaltA()`) and wait 1 second before the next scan

### 10.4 UID Extraction Code

```cpp
String cardUID = "";
for (byte i = 0; i < rfid.uid.size; i++) {
  if (rfid.uid.uidByte[i] < 0x10) cardUID += "0";  // zero-pad
  cardUID += String(rfid.uid.uidByte[i], HEX);
}
cardUID.toUpperCase();
```

---

## 11. Node 6: RFID Simulator (Flutter App)

### 11.1 Purpose

The RFID Simulator is a **software replacement** for the ESP32 RFID hardware node. It uses the phone's built-in NFC hardware to scan RFID/NFC tags and publishes the tag UID to the **same MQTT topic** (`smartHome/devices/door/testCard`) with the **same JSON payload format** (`{"cardTag": "..."}`) as the ESP32 hardware node. From the backend's perspective, the two are **functionally identical**.

### 11.2 Technology Stack

| Component | Package | Version |
|-----------|---------|---------|
| NFC Reading | `nfc_manager` | ^3.5.0 |
| MQTT Client | `mqtt_client` | ^10.8.0 |
| Typography | `google_fonts` | ^6.2.1 |
| Framework | Flutter | SDK >= 3.11.5 |

### 11.3 Functional Equivalence with ESP32

| Aspect | ESP32 RFID | RFID Simulator |
|--------|-----------|----------------|
| Tag Reading | MFRC522 via SPI | Phone NFC via nfc_manager |
| UID Format | Uppercase hex string | Uppercase hex string |
| MQTT Topic | `smartHome/devices/door/testCard` | `smartHome/devices/door/testCard` |
| Payload | `{"cardTag": "<UID>"}` | `{"cardTag": "<UID>"}` |
| MQTT Library | PubSubClient (C++) | mqtt_client (Dart) |
| QoS | 0 | 1 (atLeastOnce) |

### 11.4 NFC Tag Discovery

The app supports multiple NFC technology types, checking in order: NfcA, NfcB, ISO 15693, ISO 7816, NDEF. For each, the `identifier` byte array is extracted and converted to uppercase hex:

```dart
String _bytesToHex(List<int> bytes) =>
    bytes.map((b) => b.toRadixString(16).padLeft(2, '0').toUpperCase()).join('');
```

### 11.5 User Interface

The app features a dark-themed glassmorphism UI with three sections:
1. **MQTT Broker Configuration** — IP address and port input fields with connect/disconnect toggle
2. **NFC Scanner** — Animated pulse indicator, start/stop scanning, last scanned tag display
3. **Scan History** — Timestamped log of the last 20 scanned tags

---

## 12. MQTT Topic Hierarchy

```
smartHome/
|-- status                                    <- Controller LWT ("online"/"offline")
|-- devices/
|   |-- lamp/
|   |   |-- lamp1/set                         <- {"set":"on"} or {"set":"off"}
|   |   +-- lamp2/set
|   |-- fan/
|   |   |-- fan1/set
|   |   +-- fan2/set
|   |-- alarm/set
|   |-- door/
|   |   |-- changePassword                    <- {"oldPassword":"1234","newPassword":"5678"}
|   |   |-- changePassword/status             <- "CHANGED" / "FAIL:..."
|   |   +-- testCard                          <- {"cardTag":"A3B2C1D0"}
|   |-- dht11/
|   |   |-- temperature/state                 <- {"value":25.50}
|   |   +-- humidity/state                    <- {"value":60.00}
|   |-- mq2/gas/state                         <- {"value":15}
|   +-- fireSensor/fire/state                 <- {"value":0}
+-- weather/
    |-- water/state                           <- {"value":45}
    |-- light/state                           <- {"value":78}
    |-- temperature/state                     <- {"value":23.5}
    +-- humidity/state                        <- {"value":55.2}
```

---

## 13. Backend Integration

### 13.1 NestJS MQTT Controller

The NestJS backend exposes REST API endpoints that translate HTTP POST requests into MQTT publications:

| Endpoint | MQTT Topic | Action |
|----------|-----------|--------|
| `POST /mqtt/setLed/lamp1` | `smartHome/devices/lamp/lamp1/set` | Toggle lamp 1 |
| `POST /mqtt/setLed/lamp2` | `smartHome/devices/lamp/lamp2/set` | Toggle lamp 2 |
| `POST /mqtt/setfan/fan1` | `smartHome/devices/fan/fan1/set` | Toggle fan 1 |
| `POST /mqtt/setfan/fan2` | `smartHome/devices/fan/fan2/set` | Toggle fan 2 |
| `POST /mqtt/setAlarm` | `smartHome/devices/alarm/set` | Toggle alarm |
| `POST /mqtt/setDoor` | Door control | Toggle door |
| `POST /mqtt/setTempTreshold` | Temperature threshold | Set threshold value |

### 13.2 RFID Verification Flow

When the ESP32 RFID Reader or the RFID Simulator publishes to `smartHome/devices/door/testCard`, the backend:

1. Receives the MQTT message via its broker subscription
2. Extracts the `cardTag` value from the JSON payload
3. Queries the user database for a matching `cardTag` field
4. If a valid user is found, grants access (publishes door unlock command)
5. If not found, denies access

```
+----------+    MQTT     +----------+    DB Query   +----------+
| ESP32    | ----------> |  NestJS  | ------------> | MongoDB  |
| or       |  testCard   |  Backend |               |          |
| Flutter  |  topic      |          | <------------ |          |
| Simulator|             |          |   User found  |          |
+----------+             +----+-----+               +----------+
                              | MQTT publish
                              v
                         +----------+
                         |Controller|
                         |Card      | -> Unlock door
                         +----------+
```

---

## 14. Data Flow Analysis

### 14.1 Actuator Command Flow (App to Physical Device)

```
Flutter App -> HTTP POST -> NestJS Backend -> MQTT Publish -> Controller Card -> GPIO -> Relay -> Device
```

### 14.2 Sensor Telemetry Flow (Physical Sensor to App)

```
DHT11/MQ-2/Flame -> Sensor Node (ESP8266) -> MQTT Publish -> Backend SSE -> Flutter App UI
```

### 14.3 Weather Data Flow (Slave to Master to Cloud)

```
Water/LDR/DHT -> Weather Slave (Uno) -> I2C Bus -> Sensor Node (ESP8266) -> MQTT -> Backend SSE -> App
```

### 14.4 Door Access Flow (Keypad)

```
User -> Keypad -> Door Card (Uno) -> UART "REQ:pin" -> Controller Card (ESP8266) -> EEPROM check -> UART "OK"/"FAIL" -> Door Card -> Solenoid/LCD
```

### 14.5 Door Access Flow (RFID)

```
RFID Card -> MFRC522/Phone NFC -> ESP32 or Flutter -> MQTT {"cardTag":"..."} -> Backend -> DB lookup -> MQTT door command -> Controller -> Solenoid
```

---

## 15. Security Considerations

### 15.1 Current Implementation

| Aspect | Status | Detail |
|--------|--------|--------|
| MQTT Authentication | None | Anonymous connections to broker |
| MQTT Encryption | None | Plain TCP on port 1883 |
| Wi-Fi Credentials | Hardcoded | In source code (config.cpp) |
| Door Password | EEPROM | 4-digit PIN stored in non-volatile memory |
| Password Change | Validated | Requires old password, 4-digit numeric new password |
| RFID Tag | UID only | No cryptographic challenge-response |

### 15.2 Recommendations

1. Enable MQTT TLS (port 8883) with client certificates
2. Use environment variables or secure provisioning for Wi-Fi/MQTT credentials
3. Implement MQTT ACLs to restrict topic access per client
4. Add RFID mutual authentication (MIFARE DESFire or similar)
5. Rate-limit password attempts to prevent brute-force attacks

---

## 16. Conclusion

This embedded system demonstrates a functional multi-node IoT architecture for home automation. The design separates concerns across specialized hardware nodes: the Controller Card manages actuators, the Door Card provides human-machine interaction, the Sensor Node aggregates telemetry, the Weather Slave extends sensor coverage over I2C, and the ESP32 RFID / RFID Simulator provides RFID-based access control. MQTT serves as the unifying communication fabric, enabling loose coupling between producers and consumers, while the NestJS backend bridges the embedded network with the mobile application layer via REST APIs and Server-Sent Events.

The RFID Simulator's functional equivalence with the ESP32 hardware node validates the system's protocol-driven design — any client capable of publishing the correct JSON payload to the designated MQTT topic can participate in the access control workflow, regardless of its hardware implementation.

---

*Report generated for academic assessment — Smart Home Embedded Systems Project, June 2026.*
