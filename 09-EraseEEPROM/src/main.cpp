#include <Arduino.h>
#include "EEPROM.h"

const uint8_t eepromAdr = 0x00;
const uint8_t adrMax = 0xFF;
const uint8_t eepromChr = 0xFF;

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  // turn the LED off by making the voltage LOW
  digitalWrite(LED_BUILTIN, LOW);
  if (!EEPROM.begin(adrMax+1)) {
    Serial.println("Failed to initialise EEPROM");
    return;
  }
  for (uint8_t adr = 0; adr < adrMax; adr++) {
    EEPROM.write (eepromAdr + adr, eepromChr);
    EEPROM.commit();
  }
  Serial.println("Succeeded in erasing EEPROM");
  // turn the LED on by making the voltage HIGH
  digitalWrite(LED_BUILTIN, HIGH);
}

void loop() {
  // nothing
}