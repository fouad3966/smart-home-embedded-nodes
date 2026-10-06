#include "password_store.h"

#include <EEPROM.h>

#include "app_globals.h"
#include "config.h"

void loadPassword() {
  char buf[PWD_LEN + 1] = {0};
  for (int i = 0; i < PWD_LEN; i++) {
    char c = static_cast<char>(EEPROM.read(PWD_ADDR + i));
    if (c < '0' || c > '9') return;
    buf[i] = c;
  }
  appState.storedPassword = String(buf);
}

void savePassword(const String& pwd) {
  for (int i = 0; i < PWD_LEN; i++) {
    EEPROM.write(PWD_ADDR + i, pwd[i]);
  }
  EEPROM.commit();
}
