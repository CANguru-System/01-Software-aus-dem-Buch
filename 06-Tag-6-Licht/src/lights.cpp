
/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <CANguru-Buch@web.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gustav Wostrack
 * ----------------------------------------------------------------------------
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <ticker.h>
#include <lights.h>
#include <licht.h>

// Anzahl der Zeilen der einzelnen Programme
uint8_t NumLEDLines[numberofLEDProgs] = {
    NumLEDLinesKnightRider, NumLEDLinesBaustelle, NumLEDLinesAmpel, NumLEDLinesBlaulicht, NumLEDLinesHaus1, NumLEDLinesHaus2, NumLEDLinesHaus3, NumLEDLinesHaus4};

// setzt die Anfangswerte
void CANguruLight::begin(uint8_t noCL, LEDPrograms lP)
{
  switch (noCL)
  {
  case 0:
  case 2:
    offsetChannel = 0;
    break;
  case 1:
  case 3:
    offsetChannel = 8;
    break;
  }
  if (noCL < 2)
    pwm = Adafruit_PWMServoDriver(0x40);
  else
    pwm = Adafruit_PWMServoDriver(0x41);
  pwm.reset();
  pwm.begin();
  pwm.setPWMFreq(500); // This is the maximum PWM frequency

  Wire.setClock(400000);
  setPrg(lP);
}

// schaltet dann die LED ein- oder aus
void CANguruLight::switchLeds()
{
  static uint16_t onT = 0;
  static uint16_t offT = 0;
  uint8_t channel = 0;
  if (dim == dimMax)
  {
    if (chgLeds)
    {
      for (uint8_t bit = 0x80; bit; bit >>= 1)
      {
        if (bit & currLeds)
        {
          ledOn(channel, onTime);
        }
        else
        {
          ledOff(channel, offTime);
        }
        channel++;
      }
      chgLeds = false;
    }
  }
  else
  {
    for (uint8_t bit = 0x80; bit; bit >>= 1)
    {
      if (bit & currLeds)
      {
        if (onT < onTime)
          onT = onT + dim;
        else
        {
          onT = 0;
        }
        ledOn(channel, onT);
      }
      else
      {
        if (offT < offTime)
          offT = offT + dim;
        else
        {
          offT = 0;
        }
        ledOff(channel, offT);
      }
      channel++;
    }
  }
}

// stellt die Daten bereit nach denen die LED geschaltet werden
void CANguruLight::Update()
{
  LEDLinestruct currValues;
  if (currmSecs == time4action)
  {
    currValues = getCurrValues(currProg, currLine);
    currLeds = currValues.LED;
    chgLeds = true;
    currLine++;
    if (currLine == numLines)
    {
      currLine = 0;
    }
    currValues = getCurrValues(currProg, currLine);
    time4action = currValues.timeInMS * (1 + speed / 10);
  }
  if (currLine == 0)
    currmSecs = 0;
  else
    currmSecs++;
}

// schaltet eine LED an
void CANguruLight::ledOn(uint8_t ch, uint16_t onT)
{
  pwm.setPWM(ch + offsetChannel, 0, onT);
}

// schaltet eine LED aus
void CANguruLight::ledOff(uint8_t ch, uint16_t offT)
{
  pwm.setPWM(ch + offsetChannel, 0, offT);
}

// liefert den Status einer Zeile eines Programmes; danach werden
// dann die LED geschaltet  
LEDLinestruct CANguruLight::getCurrValues(LEDPrograms ledProg, uint8_t line)
{
  LEDLinestruct cV;
  switch (ledProg)
  {
  case KnightRider:
    cV.timeInMS = LEDLineKR[line].timeInMS;
    cV.LED = LEDLineKR[line].LED;
    break;
  case Baustelle:
    cV.timeInMS = LEDLineBaustelle[line].timeInMS;
    cV.LED = LEDLineBaustelle[line].LED;
    break;
  case Ampel:
    cV.timeInMS = LEDLineAmpel[line].timeInMS;
    cV.LED = LEDLineAmpel[line].LED;
    break;
  case Blaulicht:
    cV.timeInMS = LEDLineBlaulicht[line].timeInMS;
    cV.LED = LEDLineBlaulicht[line].LED;
    break;
  case Haus1:
    cV.timeInMS = LEDLineHaus1[line].timeInMS;
    cV.LED = LEDLineHaus1[line].LED;
    break;
  case Haus2:
    cV.timeInMS = LEDLineHaus2[line].timeInMS;
    cV.LED = LEDLineHaus2[line].LED;
    break;
  case Haus3:
    cV.timeInMS = LEDLineHaus3[line].timeInMS;
    cV.LED = LEDLineHaus3[line].LED;
    break;
  case Haus4:
    cV.timeInMS = LEDLineHaus4[line].timeInMS;
    cV.LED = LEDLineHaus4[line].LED;
    break;
  case LastLEDProg:
    break;
  }
  return cV;
}

// stellt die Dunkelheit ein
void CANguruLight::setoffTime(uint16_t b)
{
  offTime = b;
  Update();
}

// stellt die Helligheit ein
void CANguruLight::setonTime(uint16_t b)
{
  onTime = b;
  Update();
}

// stellt die Geschwindigkeit ein
void CANguruLight::setSpeed(uint8_t sp)
{
  speed = sp;
  setPrg(currProg);
}

// stellt den DIM-Faktor ein
void CANguruLight::setDim(uint16_t d)
{
  dim = d;
}

// stellt das Programm ein
void CANguruLight::setPrg(LEDPrograms lP)
{
  currProg = lP;
  numLines = NumLEDLines[currProg];
  currmSecs = 0;
  currLine = 0;
  chgLeds = false;
  time4action = 0;
  for (uint8_t ch = 0; ch < 8; ch++)
    ledOff(ch + offsetChannel, 0);
}
