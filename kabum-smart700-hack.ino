#include <Arduino.h>
#include "Smart700.h"
#include "esp_task_wdt.h"
#include "Setup.h"

Smart700 smart700;

void setup() {
#ifdef DEBUG_MODE
  Serial.begin(115200);
  while (!Serial) delay(10);  // aguarda USB CDC conectar
#endif
  smart700.begin();
  esp_task_wdt_init(300, true);  // 300s = 5min
  esp_task_wdt_add(NULL);
  delay(500);
}

void loop() {
  static bool awaitingCharging = false;
  if (!smart700.shutdown() && smart700.searchingForCharger() && !smart700.WiFi() && !smart700.charging() && !awaitingCharging) {
    smart700.restart();
    awaitingCharging = true;
  } else if ((awaitingCharging && smart700.charging()) || smart700.shutdown())
    awaitingCharging = false;

  smart700.tick();
  esp_task_wdt_reset();

  delay(1000);
}