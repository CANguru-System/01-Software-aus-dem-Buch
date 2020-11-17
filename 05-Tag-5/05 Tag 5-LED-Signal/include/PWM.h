
/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <CANguru-Buch@web.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gustav Wostrack
 * ----------------------------------------------------------------------------
 */

#pragma once

#include "Arduino.h"

enum colorLED
{
  red,
  green
};

/* Märklin Dokumentation:
Bit 0,1: Stellung:
00: Aus, Rund, Rot, Rechts, HP0
01: Ein, Grün, Gerade, HP1
10: Gelb, Links, HP2
11: Weiss, SH0
*/
//

const int freq = 5000;
const int resolution = 8;

// Verzögerungen
const uint8_t minsignaldelay = 2;
const uint8_t maxsignaldelay = 99;
const uint8_t stdsignaldelay = (minsignaldelay + maxsignaldelay - 1) / 2;

class LEDSignal
{
public:
  // Voreinstellungen, Servonummer wird physikalisch mit
  // einem Signal verbunden;
  void Attach(uint8_t signalPinGREEN, uint8_t signalPinRED, uint8_t ch);
  // Setzt die Zielfarbe
  void SetcolorLED();
  // Zielfarbe ist Grün
  void GoGreen();
  // Zielfarbe ist Rot
  void GoRed();
  // Überprüft periodisch, ob die Zielfarbe erreicht ist
  void Update();
  // Setzt die Zielfarbe
  void SetLightDest(colorLED c);
  // Liefert die Zielfarbe
  colorLED GetLightDest();
  // Liefert die aktuelle Farbe
  colorLED GetLightCurr();
  // Setzt die aktuelle Farbe
  void SetLightCurr(colorLED c);
  // Damit werden unnötige Farbänderungen vermieden
  bool ColorChg();
  // Setzt den Wert für die Verzögerung
  void SetDelay(int d);
  // Setzt die Adresse eines Signals
  void Set_to_address(uint16_t _to_address);
  // Liefert die Adresse eines Signals
  uint16_t Get_to_address();

private:
  uint8_t pinGREEN, pinRED;
  uint8_t green_dutyCycle_dest, green_dutyCycle_curr;
  uint8_t red_dutyCycle_dest, red_dutyCycle_curr;
  uint8_t green_channel, red_channel;
  uint8_t green_increment, red_increment;
  unsigned long updateInterval; // interval between updates
  unsigned long lastUpdate;     // last update of colorLED
  uint16_t acc__to_address;
  colorLED acc_light_dest;
  colorLED acc_light_curr;
  colorLED wakeupdir;
};