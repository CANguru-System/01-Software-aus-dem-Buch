#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include "EEPROM.h"
#include <ArduinoOTA.h>

const uint8_t numChars = 32;
// EEPROM-Adressen
#define setup_done 0x47
// EEPROM-Belegung
// EEPROM-Speicherplätze der Local-IDs
const uint16_t adr_setup_done = 0x00;
const uint16_t adr_ota = adr_setup_done + 0x01;
const uint16_t adr_ssid = adr_ota + 0x01;
const uint16_t adr_password = adr_ssid + numChars;
const uint16_t adr_IP0 = adr_password + numChars;
const uint16_t adr_IP1 = adr_IP0 + 0x01;
const uint16_t adr_IP2 = adr_IP1 + 0x01;
const uint16_t adr_IP3 = adr_IP2 + 0x01;
const uint16_t lastAdr0 = adr_IP3 + 0x01;

const uint8_t startWithOTA = 0x77;
const uint8_t startWithoutOTA = 0x55;

void Connect2WiFiandOTA();
