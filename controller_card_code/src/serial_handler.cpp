#include "serial_handler.h"

#include "app_globals.h"
#include "config.h"

void handleSerial() {
  static String buf = "";
  static unsigned long lastChar = 0;

  if (buf.length() > 0 && millis() - lastChar > 500) {
    buf = "";
  }

  while (Serial.available()) {
    char c = static_cast<char>(Serial.read());
    lastChar = millis();

    if (c == '\n') {
      buf.trim();
      if (buf.startsWith("REQ:")) {
        String input = buf.substring(4);
        input.trim();

        if (input == appState.storedPassword) {
          Serial.println("OK");
        } else {
          Serial.println("FAIL");
        }
      }
      buf = "";
    } else {
      buf += c;
    }
  }
}
