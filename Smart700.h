#ifndef _SMART700_H_
#define _SMART700_H_

#include "Setup.h"
#include <Arduino.h>


class Smart700 {
private:
  enum ChargeState {
    UNKNOWN,
    EXPECT_HIGH,
    EXPECT_LOW
  };

  ChargeState chargeState = UNKNOWN;

  bool _wifi = false;
  bool _shutdown = false;
  bool _searchingForCharger = false;
  bool _charging = false;

  const uint8_t analogPins[4] = { 1, 2, 3, 4 };
  const uint8_t digitalOutPins[2] = { 5, 6 };


  bool between(uint16_t v, uint16_t min, uint16_t max) {
    return (v >= min && v <= max);
  }

  void detectCharging(int a1, int a0) {
    static uint8_t oscillationCount = 0;
    bool a1High = a1 > 2000;
    bool a1Low = a1 < 100;

    bool a0High = a0 > 2000;
    bool a0Low = a0 < 100;

    if (!a0High) {
      _charging = false;
      oscillationCount = 0;
      chargeState = UNKNOWN;
      return;
    }

    switch (chargeState) {

      case UNKNOWN:
        if (a1High) chargeState = EXPECT_LOW;
        else if (a1Low) chargeState = EXPECT_HIGH;
        break;

      case EXPECT_HIGH:
        if (a1High) {
          oscillationCount++;
          chargeState = EXPECT_LOW;
        }
        break;

      case EXPECT_LOW:
        if (a1Low) {
          oscillationCount++;
          chargeState = EXPECT_HIGH;
        }
        break;
    }

    if (oscillationCount >= 4)
      _charging = true;
    else
      _charging = false;
  }

  void WiFiCheck(uint16_t a3) {
    static int8_t stableCount = 0;  // Contador de estabilidade
    static int8_t lastZone = -1;
    int8_t zone;

    if (a3 < 300) {
      zone = 0;  // zona baixa → A3 ativo
    } else if (a3 > 2000) {
      zone = 1;  // zona alta → A3 inativo
    } else {
      zone = -1;  // zona indefinida → ignora, mas não troca estado
    }

    // Se ficou em uma zona válida
    if (zone == lastZone && zone != -1) {
      stableCount++;
    } else {
      stableCount = 1;  // reset para novo estado
      lastZone = zone;
    }

    // Se está estável por tempo suficiente, confirma mudança
    if (stableCount >= 2) {

      if (zone == 0) {
        _wifi = true;
      } else if (zone == 1) {
        _wifi = false;
      }
    }
  }

  void detectBoardShutdown(uint16_t a3, uint16_t a2, uint16_t a1, uint16_t a0) {
    static uint8_t shutdownStableCount = 0;

    bool match =
      (a2 > 2000) && between(a3, 0, 900) && between(a1, 0, 900) && between(a0, 0, 900);

    if (match) {
      shutdownStableCount++;
    } else {
      shutdownStableCount = 0;
    }

    if (shutdownStableCount >= 3)
      _shutdown = true;
    else
      _shutdown = false;
  }

  void detectSearchingForCharger(uint16_t a1, uint16_t a2) {
    static uint8_t searchStableCount = 0;

    bool match = (a1 < 500) && (a2 < 500);

    if (match) {
      searchStableCount++;
    } else {
      searchStableCount = 0;
    }

    if (searchStableCount >= 3)
      _searchingForCharger = true;
    else
      _searchingForCharger = false;
  }

public:
  bool WiFi() {
    return _wifi;
  }

  bool shutdown() {
    return _shutdown;
  }

  bool searchingForCharger() {
    return _searchingForCharger;
  }

  bool charging() {
    return _charging;
  }

  void goToCharger() {
    digitalWrite(digitalOutPins[1], 0);
    delay(250);
    digitalWrite(digitalOutPins[1], 1);
  }

  void restart() {
    digitalWrite(digitalOutPins[0], 1);
    delay(4000);
    digitalWrite(digitalOutPins[0], 0);
    delay(5000);
    digitalWrite(digitalOutPins[0], 1);
    delay(2000);
    digitalWrite(digitalOutPins[0], 0);
    delay(15000);

    goToCharger();
  }

  void begin() {
    for (uint8_t i = 0; i < sizeof(analogPins) / sizeof(analogPins[0]); i++)
      pinMode(analogPins[i], INPUT);

    pinMode(digitalOutPins[0], OUTPUT);
    digitalWrite(digitalOutPins[0], 0);
    pinMode(digitalOutPins[1], OUTPUT);
    digitalWrite(digitalOutPins[1], 1);

    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);
  }

  void tick() {
    uint16_t a3_raw = (uint16_t)analogRead(analogPins[0]);
    uint16_t a2_raw = (uint16_t)analogRead(analogPins[1]);
    uint16_t a1_raw = (uint16_t)analogRead(analogPins[2]);
    uint16_t a0_raw = (uint16_t)analogRead(analogPins[3]);

    detectCharging(a1_raw, a0_raw);
    WiFiCheck(a3_raw);
    detectBoardShutdown(a3_raw, a2_raw, a1_raw, a0_raw);
    detectSearchingForCharger(a1_raw, a2_raw);


#ifdef DEBUG_MODE
    Serial.println("------- SMART700 DEBUG -------");

    Serial.printf("Board State:      %s\n", _shutdown ? "OFF" : "ON");
    Serial.printf("WiFi:             %s\n", _wifi ? "ACTIVE" : "INACTIVE");
    Serial.printf("Charging:         %s\n", _charging ? "YES" : "NO");
    Serial.printf("SearchingCharge:  %s\n", _searchingForCharger ? "YES" : "NO");

    Serial.printf("A3 Raw: %u\n", a3_raw);
    Serial.printf("A2 Raw: %u\n", a2_raw);
    Serial.printf("A1 Raw: %u\n", a1_raw);
    Serial.printf("A0 Raw: %u\n", a0_raw);

    Serial.println("Voltages:");
    for (int i = 0; i < 4; i++) {
      int raw = analogRead(analogPins[i]);
      float volts = raw * 3.3 / 4095.0;
      Serial.printf("A%d (GPIO%d): %4d (%.2f V)\n",
                    3 - i, analogPins[i], raw, volts);
    }

    Serial.println("--------------------------------\n");
#endif
  }
};

#endif