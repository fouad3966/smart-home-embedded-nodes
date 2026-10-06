#include "app_globals.h"

WiFiClient espClient;
PubSubClient mqtt(espClient);
AppState appState;
