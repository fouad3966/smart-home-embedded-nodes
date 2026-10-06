#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define LOCK_PIN 12
#define BUZZER 2

String input = "";
bool waiting = false;
unsigned long requestTime = 0;

// ---------- KEYPAD ----------
const byte ROWS = 4;
const byte COLS = 3;

char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};

byte rowPins[ROWS] = {9,8,7,6};
byte colPins[COLS] = {5,4,3};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ---------- SETUP ----------
void setup() {

  Serial.begin(9600);

  pinMode(LOCK_PIN, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  digitalWrite(LOCK_PIN, LOW);

  lcd.init();
  lcd.backlight();
  lcd.print("Enter Pass:");
}

// ---------- LOOP ----------
void loop() {

  // ----- KEYPAD INPUT -----
  char key = keypad.getKey();

  if (key && !waiting) {

    if (key == '*') {
      input = "";
      lcd.clear();
      lcd.print("Enter Pass:");
    }

    else if (key == '#') {

      if (input.length() == 4) {

        Serial.println("REQ:" + input);
        waiting = true;
        requestTime = millis();

        lcd.clear();
        lcd.print("Checking...");
      }

      input = "";
    }

    else {
      if (input.length() < 4) {
        input += key;
        lcd.setCursor(input.length() - 1, 1);
        lcd.print("*");
      }
    }
  }

  // ----- TIMEOUT -----
  if (waiting && millis() - requestTime > 3000) {
    lcd.clear();
    lcd.print("NO RESPONSE");
    delay(1500);
    lcd.clear();
    lcd.print("Enter Pass:");
    waiting = false;
  }

  // ----- SERIAL RESPONSE -----
  if (Serial.available() && waiting) {

    String res = "";

    while (Serial.available()) {
      char c = Serial.read();
      if (c == '\n') break;
      res += c;
      delay(2);
    }

    res.trim();

    lcd.clear();

    if (res == "OK") {

      lcd.print("ACCESS GRANTED");

      digitalWrite(LOCK_PIN, HIGH);
      tone(BUZZER, 1000, 200);

      delay(3000);

      digitalWrite(LOCK_PIN, LOW);
    }

    else if (res == "FAIL") {

      lcd.print("ACCESS DENIED");

      for (int i = 0; i < 3; i++) {
        tone(BUZZER, 400, 150);
        delay(200);
      }
    }

    delay(1500);

    lcd.clear();
    lcd.print("Enter Pass:");
    waiting = false;
  }
  }