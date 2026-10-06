#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <MFRC522.h>

// ---------------- WIFI ----------------
const char* ssid = "Mnwr's M14";
const char* password = "mnwrameur20044";

// ---------------- MQTT ----------------
const char* mqtt_server = "10.228.191.110"; // MQTT broker IP
const int mqtt_port = 1883;

const char* mqtt_topic = "smartHome/devices/door/testCard";

// ---------------- RFID ----------------
#define SS_PIN 5
#define RST_PIN 21

MFRC522 rfid(SS_PIN, RST_PIN);

// ---------------- CLIENTS ----------------
WiFiClient espClient;
PubSubClient client(espClient);

// ---------------- WIFI CONNECT ----------------
void setup_wifi() {

  delay(10);

  Serial.println();
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.localIP());
}

// ---------------- MQTT CONNECT ----------------
void reconnectMQTT() {

  while (!client.connected()) {

    Serial.print("Connecting to MQTT...");

    String clientId = "ESP32-RFID-";

    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {

      Serial.println("connected");

    } else {

      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying...");

      delay(2000);
    }
  }
}

// ---------------- SETUP ----------------
void setup() {

  Serial.begin(115200);

  SPI.begin();
  rfid.PCD_Init();

  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);

  Serial.println("Scan RFID card...");
}

// ---------------- LOOP ----------------
void loop() {

  if (!client.connected()) {
    reconnectMQTT();
  }

  client.loop();

  // Check for card
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  // Read card
  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  String cardUID = "";

  for (byte i = 0; i < rfid.uid.size; i++) {

    if (rfid.uid.uidByte[i] < 0x10) {
      cardUID += "0";
    }

    cardUID += String(rfid.uid.uidByte[i], HEX);
  }

  cardUID.toUpperCase();

  Serial.print("Card UID: ");
  Serial.println(cardUID);

  // Create JSON payload
  String payload = "{\"cardTag\":\"" + cardUID + "\"}";

  // Publish MQTT
  client.publish(mqtt_topic, payload.c_str());

  Serial.print("MQTT Sent: ");
  Serial.println(payload);

  rfid.PICC_HaltA();

  delay(1000);
}