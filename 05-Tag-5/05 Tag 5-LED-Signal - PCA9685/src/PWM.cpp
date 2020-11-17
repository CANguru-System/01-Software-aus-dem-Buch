
/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <CANguru-Buch@web.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gustav Wostrack
 * ----------------------------------------------------------------------------
 */
#include <PWM.h>
#include <Adafruit_PWMServoDriver.h>

// Voreinstellungen, Servonummer wird physikalisch mit
// einem Signal verbunden;
void LEDSignal::Attach(uint8_t ch)
{
  wakeupdir = red;
  // configure LED PWM functionalitites
  green_channel = ch * 2;
  red_channel = ch * 2 + 1;
// called this way, it uses the default address 0x40
  pwm_green = Adafruit_PWMServoDriver();
  pwm_red = Adafruit_PWMServoDriver();
  pwm_green.begin();
  pwm_red.begin();
  pwm_green.setPWMFreq(freq); // This is the maximum PWM frequency
  pwm_red.setPWMFreq(freq); // This is the maximum PWM frequency
}

// Setzt die Zielfarbe
void LEDSignal::SetcolorLED()
{
  lastUpdate = micros();
  acc_light_dest = acc_light_curr;
  switch (acc_light_dest)
  {
  case green:
  {
    GoGreen();
  }
  break;
  case red:
  {
    GoRed();
  }
  break;
  }
}

// Zielfarbe ist Grün
void LEDSignal::GoGreen()
{
  // es ist ROT
  // jetzt umschalten auf GRÜN
  green_dutyCycle_dest = 255;
  green_dutyCycle_curr = 0;
  green_increment = 1;
  red_dutyCycle_dest = 0;
  red_dutyCycle_curr = 255;
  red_increment = -1;
}

// Zielfarbe ist Rot
void LEDSignal::GoRed()
{
  // es ist GRÜN
  // jetzt umschalten auf ROT
  red_dutyCycle_dest = 255;
  red_dutyCycle_curr = 0;
  red_increment = 1;
  green_dutyCycle_dest = 0;
  green_dutyCycle_curr = 255;
  green_increment = -1;
}

  // Überprüft periodisch, ob die Zielfarbe erreicht ist
void LEDSignal::Update()
{
  if ((micros() - lastUpdate) > updateInterval)
  { // time to update
    // GRÜN
    if (green_dutyCycle_curr != green_dutyCycle_dest)
    {
      green_dutyCycle_curr += green_increment;
      pwm_green.setPWM(green_channel, 0, green_dutyCycle_curr);
    }
    // ROT
    if (red_dutyCycle_curr != red_dutyCycle_dest)
    {
      red_dutyCycle_curr += red_increment;
      pwm_red.setPWM(red_channel, 0, red_dutyCycle_curr);
    }
    lastUpdate = micros();
  }
}

// Setzt die Zielfarbe
void LEDSignal::SetLightDest(colorLED c)
{
  acc_light_dest = c;
}

// Liefert die Zielfarbe
colorLED LEDSignal::GetLightDest()
{
  return acc_light_dest;
}

// Liefert die aktuelle Farbe
colorLED LEDSignal::GetLightCurr()
{
  return acc_light_curr;
}

// Setzt die aktuelle Farbe
void LEDSignal::SetLightCurr(colorLED c)
{
  acc_light_curr = c;
}

// Damit werden unnötige Farbänderungen vermieden
bool LEDSignal::ColorChg()
{
  return acc_light_dest != acc_light_curr;
}

  // Setzt den Wert für die Verzögerung
void LEDSignal::SetDelay(int d)
{
  updateInterval = d * 100;
}

// Setzt die Adresse eines Signals
void LEDSignal::Set_to_address(uint16_t _to_address)
{
  acc__to_address = _to_address;
}

// Liefert die Adresse eines Signals
uint16_t LEDSignal::Get_to_address()
{
  return acc__to_address;
}
