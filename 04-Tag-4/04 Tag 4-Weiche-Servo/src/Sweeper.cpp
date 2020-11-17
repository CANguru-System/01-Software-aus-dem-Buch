
/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <CANguru-Buch@web.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gustav Wostrack
 * ----------------------------------------------------------------------------
 */

#include <Sweeper.h>
#include "device2use.h"

// Voreinstellungen, Servonummer wird physikalisch mit
// einem Servo verbunden
void Sweeper::Attach(byte servoPin, byte ch)
{
  channel = ch;
  ledcSetup(ch, 50, 16); // channel X, 50 Hz, 16-bit depth
  ledcAttachPin(servoPin, channel);
  switch (acc_pos_curr)
  {
  case right:
    pos = rightpos;
    break;
  case left:
    pos = leftpos;
    break;
  }
#ifdef formsignal
    maxbobs  = 6;
#endif
}

// Verbindung zum Servo wird gelöst
void Sweeper::Detach(byte pin)
{
  ledcDetachPin(pin);
}

// Setzt die Zielposition
void Sweeper::SetPosition()
{
  lastUpdate = micros();
  acc_pos_dest = acc_pos_curr;
  switch (acc_pos_dest)
  {
  case left:
  {
    GoLeft();
  }
  break;
  case right:
  {
    GoRight();
  }
  break;
  }
}

// Zielposition ist links
void Sweeper::GoLeft()
{
  destpos = leftpos;
// von pos < 74 bis 74, des halb increment positiv
#ifdef armservo
  increment = -1;
  endpos = -maxendpos; // +1
#endif
#ifdef linearservo
  increment = -1;
  endpos = -maxendpos; // +1
#endif
#ifdef formsignal
  increment = -1;
  endpos = 0;
  bob = maxbobs;
#endif
  way = longway;
}

// Zielposition ist rechts
void Sweeper::GoRight()
{
  destpos = rightpos; // 1
// von pos > 1 bis 1, des halb increment negativ
#ifdef armservo
  increment = 1;
  endpos = maxendpos; // -1
#endif
#ifdef linearservo
  increment = 1;
  endpos = maxendpos; // -1
#endif
#ifdef formsignal
  increment = 1;
  endpos = 0;
  bob = maxbobs;
#endif
  way = longway;
}

// Überprüft periodisch, ob die Zielposition erreicht wird
void Sweeper::Update()
{
#ifdef armservo
  const uint8_t wakeupincr = 0;
  // Überprüfung, ob die Zielposition bereits erreicht wurde
  if (pos != (destpos + endpos))
  {
    // wenn die Zeit updateInterval vorüber ist, wird
    // wird das Servo ggf. neu gestellt
    if ((micros() - lastUpdate) > updateInterval)
    {
      // time to update
      pos += increment;
      ledcWrite(channel, pos);
      lastUpdate = micros();
    }
  }
  else
  {
    // Zielposition wurde erreicht
    if (way == longway)
    {
      // Umschalten auf noway
      increment *= -1;
      endpos = 0;
      way = noway;
    }
    if (way == noway)
    {
      wakeuptimer++;
      if (wakeuptimer >= 60000)
      {
        wakeuptimer = 0;
        switch (wakeupdir)
        {
        case right:
        {
          increment = -wakeupincr;
          wakeupdir = left;
        }
        break;
        case left:
        {
          increment = wakeupincr;
          wakeupdir = right;
        }
        break;
        }
        destpos += increment;
      }
    }
  }
#endif
#ifdef linearservo
  const uint8_t wakeupincr = 0;
  // Überprüfung, ob die Zielposition bereits erreicht wurde
  if (pos != (destpos + endpos))
  {
    // wenn die Zeit updateInterval vorüber ist, wird
    // wird das Servo ggf. neu gestellt
    if ((micros() - lastUpdate) > updateInterval)
    {
      // time to update
      pos += increment;
      ledcWrite(channel, pos);
      lastUpdate = micros();
    }
  }
  else
  {
    // Zielposition wurde erreicht
    if (way == longway)
    {
      // Umschalten auf noway
      increment *= -1;
      endpos = 0;
      way = noway;
    }
    if (way == noway)
    {
      wakeuptimer++;
      if (wakeuptimer >= 60000)
      {
        wakeuptimer = 0;
        switch (wakeupdir)
        {
        case right:
        {
          increment = -wakeupincr;
          wakeupdir = left;
        }
        break;
        case left:
        {
          increment = wakeupincr;
          wakeupdir = right;
        }
        break;
        }
        destpos += increment;
      }
    }
  }
#endif

#ifdef formsignal
  if (pos != (destpos - endpos))
  {
    if ((micros() - lastUpdate) > updateInterval)
    { // time to update
      pos += increment;
      ledcWrite(channel, pos);
      lastUpdate = micros();
    }
  }
  else
  {
    if (way != noway)
    {
      bob--;
      if (bob == 0)
      {
        way = noway;
      }
      switch (way)
      {
      case longway:
      {
        way = shortway;
        endpos = maxendpos * bob * increment;
      }
      break;
      case shortway:
      {
        way = longway;
        endpos = 0;
      }
      break;
      case noway:
        break;
      }
      increment *= -1;
    }
  }
#endif
}

// Setzen der Winkel des Servo
void Sweeper::SetAngle(uint8_t end, uint8_t startAngle, uint8_t stoppAngle)
{
  const uint16_t count_low = 0;
#ifdef armservo
  const uint16_t count_high = 7200;
  const uint16_t count_base = 2200;
#endif
#ifdef linearservo
  const uint16_t count_high = 4100;
  const uint16_t count_base = 2600;
#endif
#ifdef formsignal
  const uint16_t count_high = 7200;
  const uint16_t count_base = 2200;
#endif
  const uint16_t count_degree = (count_high - count_low) / 180;
  leftpos = count_base + startAngle * count_degree;  // 5 je kleiner desto weiter nach rechts
  rightpos = count_base + stoppAngle * count_degree; // 74 je groesser desto weiter nach links
#ifdef armservo
  maxendpos = end * count_degree;
#endif
#ifdef linearservo
  maxendpos = end * count_degree;
#endif
#ifdef formsignal
  maxendpos = end * count_degree / 4;
#endif
}

// Setzt die Zielposition
void Sweeper::SetPosDest(position p)
{
  acc_pos_dest = p;
}

// Liefert die Zielposition
position Sweeper::GetPosDest()
{
  return acc_pos_dest;
}

// Setzt die aktuelle Position
void Sweeper::SetPosCurr(position p)
{
  acc_pos_curr = p;
}

// Liefert die aktuelle Position
position Sweeper::GetPosCurr()
{
  return acc_pos_curr;
}

// Hat sich die Position des Servos verändert?
// Damit werden unnötige Positionsänderungen vermieden
bool Sweeper::PosChg()
{
  return acc_pos_dest != acc_pos_curr;
}

// Setzt den Wert für die Verzögerung
void Sweeper::SetDelay(int d)
{
#ifdef armservo
  updateInterval = d * 100;
#endif
#ifdef linearservo
  updateInterval = d * 100;
#endif
#ifdef formsignal
  updateInterval = d * 200;
#endif
}

// Setzt die Adresse eines Servos
void Sweeper::Set_to_address(uint16_t _to_address)
{
  acc__to_address = _to_address;
}

// Liefert die Adresse eines Servos
uint16_t Sweeper::Get_to_address()
{
  return acc__to_address;
}
