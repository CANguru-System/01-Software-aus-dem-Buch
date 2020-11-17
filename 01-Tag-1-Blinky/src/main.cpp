
/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <CANguru-Buch@web.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gustav Wostrack
 * ----------------------------------------------------------------------------
 */

#include <Arduino.h>
#include "EEPROM.h"
#include <Ticker.h>

// EEPROM-Adressen
#define setup_done 0x47
// EEPROM-Belegung
// EEPROM-Speicherplätze der Local-IDs
const uint16_t adr_setup_done = 0x00;
const uint16_t adr_count = 0x01;
const uint16_t lastAdr = adr_count + 1;
const uint16_t EEPROM_SIZE = lastAdr;

Ticker tckr;
const float tckrTime = 0.05;

//*********************************************************************************************************
void timer1s()
{
  static uint8_t secs = 0;
  static uint8_t currcount = 0;
  secs++;
  if (secs >= 1 / tckrTime)
  {
    secs = 0;
    // der aktuelle Zählerstand wird eingelesen
    currcount = EEPROM.read(adr_count);
    // und ausgegeben
    Serial.println(EEPROM.read(adr_count));
  }
  if (currcount > 0)
  {
    if (currcount % 2 == 0)
    {
      // bei geradem Zählerstand LED an
      digitalWrite(LED_BUILTIN, LOW);
    }
    else
    {
      // bei ungeradem Zählerstand LED aus
      digitalWrite(LED_BUILTIN, HIGH);
    }
    currcount--;
  }
  else
  {
    digitalWrite(LED_BUILTIN, LOW);
  }
}

void stillAliveBlinkSetup()
{
  // der Timer wird installiert
  tckr.attach(tckrTime, timer1s); // each sec
  // initialize LED digital pin as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  // turn the LED off by making the voltage LOW
  digitalWrite(LED_BUILTIN, LOW);
}

void setup()
{
  Serial.begin(115200);
  Serial.println("\r\n\r\nB l i n k y");
  if (!EEPROM.begin(EEPROM_SIZE))
  {
    Serial.println("failed to initialise EEPROM");
  }
  uint8_t setup_todo = EEPROM.read(adr_setup_done);
  if (setup_todo != setup_done)
  {
    // wurde das Setup bereits einmal durchgeführt?
    // dann wird dieser Anteil übersprungen
    // 47, weil das EEPROM (hoffentlich) nie ursprünglich diesen Inhalt hatte

    // count anfangs auf 1 setzen
    EEPROM.write(adr_count, 1);
    EEPROM.commit();

    // setup_done auf "TRUE" setzen
    EEPROM.write(adr_setup_done, setup_done);
    EEPROM.commit();
  }
  else
  {
    // Anweisungen werden nur ab
    // dem zweiten Durchlauf ausgeführt
    // lies den Wert für count ein
    uint8_t count = EEPROM.read(adr_count);
    // und erhöhe den Wert um 1
    count += 2;
    if (count >= 1 / tckrTime)
      count = 1;
    // speichere count wieder
    EEPROM.write(adr_count, count);
    EEPROM.commit();
  }
  // ab hier werden die Anweisungen bei jedem Start durchlaufen
  stillAliveBlinkSetup();
}

void loop()
{
  // put your main code here, to run repeatedly:
}