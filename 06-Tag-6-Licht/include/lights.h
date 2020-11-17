
/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <CANguru-Buch@web.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gustav Wostrack
 * ----------------------------------------------------------------------------
 */

#ifndef _LIGHTS_H
#define _LIGHTS_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include "licht.h"

enum LEDPrograms
{
  KnightRider,
  Baustelle,
  Ampel,
  Blaulicht,
  Haus1,
  Haus2,
  Haus3,
  Haus4,
  LastLEDProg
};

const uint8_t numberofLEDProgs = LastLEDProg;

// Kanalnummer #1 - Dunkelphase
const uint16_t offTimeMin = 0x00;
const uint16_t offTimeMax = 0x0FFF;
const uint16_t offTimeStart = offTimeMin;
// Kanalnummer #2 - Helligkeit
const uint16_t onTimeMin = 0x00;
const uint16_t onTimeMax = 0x0FFF;
const uint16_t onTimeStart = onTimeMax;
// Kanalnummer #3 - Dim
const uint16_t dimMin = 0x08;
const uint16_t dimMax = 0x0400;
const uint16_t dimStart = dimMax;
// Kanalnummer #4 - Geschwindigkeit
const uint16_t speedMin = 0x00;
const uint16_t speedMax = 0x0020;
const uint16_t speedStart = speedMin;
// Kanalnummer #5 ff- LEDProgramm für die einzelnen Expander festlegen
const uint16_t progMin = 0x00;
const uint16_t progMax = numberofLEDProgs-1;

class CANguruLight
{
public:
  // setzt die Anfangswerte
  void begin(uint8_t noCL, LEDPrograms lP);
  // schaltet dann die LED ein- oder aus
  void switchLeds();
  // stellt die Daten bereit nach denen die LED geschaltet werden
  void Update();
  // stellt die Dunkelheit ein
  void setoffTime(uint16_t b);
  // stellt die Helligkeit ein
  void setonTime(uint16_t b);
  // stellt die Geschwindigkeit ein
  void setSpeed(uint8_t sp);
  // stellt den DIM-Faktor ein
  void setDim(uint16_t d);
  // stellt das Programm ein
  void setPrg(LEDPrograms lP);

private:
  // schaltet eine LED an
  void ledOn(uint8_t ch, uint16_t onT);
  // schaltet eine LED aus
  void ledOff(uint8_t ch, uint16_t offT);
  // liefert den Status einer Zeile eines Programmes; danach werden
  // dann die LED geschaltet
  LEDLinestruct getCurrValues(LEDPrograms ledProg, uint8_t line);

  Adafruit_PWMServoDriver pwm;
  LEDPrograms currProg;
  uint8_t numLines;
  uint8_t currLine;
  unsigned long currmSecs;
  unsigned long time4action;
  uint8_t currLeds;
  bool chgLeds;
  uint8_t speed;
  uint16_t onTime;
  uint16_t offTime;
  uint16_t dim;
  uint8_t offsetChannel;
};

#endif
