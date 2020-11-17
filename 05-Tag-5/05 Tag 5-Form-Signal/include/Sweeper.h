
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
#include "device2use.h"

// mögliche Positionen des Servos
enum position
{
  right,
  left
};

/* Märklin Dokumentation:
Bit 0,1: Stellung:
00: Aus, Rund, Rot, Rechts, HP0
01: Ein, Grün, Gerade, HP1
10: Gelb, Links, HP2
11: Weiss, SH0
*/
// 
enum enumway
{
  longway,    // von rechts nach links oder umgekehrt
  shortway,   // der kleine Überschwinger beim Signal
  noway       // Servo steht
};

// Verzögerungen
const uint8_t maxservodelay = 100;
const uint8_t minservodelay = maxservodelay / 10;
const uint8_t stdservodelay = maxservodelay / 10;

class Sweeper
{
public:
  // Voreinstellungen, Servonummer wird physikalisch mit
  // einem Servo verbunden;
  void Attach(byte servoPin, byte channel);
  // Verbindung zum Servo wird gelöst
  void Detach(byte pin);
  // Setzt die Zielposition
  void SetPosition();
  // Zielposition ist links
  void GoLeft();
  // Zielposition ist rechts
  void GoRight();
  // Überprüft periodisch, ob die Zielposition erreicht wird
  void Update();
  // Setzen der Winkel des Servo
  void SetAngle(uint8_t end, uint8_t startAngle, uint8_t stoppAngle);
  // Setzt die Zielposition
  void SetPosDest(position p);
  // Liefert die Zielposition
  position GetPosDest();
  // Setzt die aktuelle Position
  void SetPosCurr(position p);
  // Liefert die aktuelle Position
  position GetPosCurr();
  // Hat sich die Position des Servos verändert?
  // Damit werden unnötige Positionsänderungen vermieden
  bool PosChg();
  // Setzt den Wert für die Verzögerung
  void SetDelay(int d);
  // Setzt die Adresse eines Servos
  void Set_to_address(uint16_t _to_address);
  // Liefert die Adresse eines Servos
  uint16_t Get_to_address();

private:
  byte channel;
  int pos;       // current servo position
  int destpos;   // servo position, where to go
  int increment; // increment to move for each interval
  int endpos;
  unsigned long updateInterval; // interval between updates
  unsigned long lastUpdate;     // last update of position
  uint16_t acc__to_address;
  position acc_pos_dest;
  position acc_pos_curr;
  int leftpos;   // 74 je groesser desto weiter nach links
  int rightpos;  // 5 je kleiner desto weiter nach rechts
  int maxendpos; // * grdinmillis;
  enumway way;
  uint16_t wakeuptimer;
  position wakeupdir;
#ifdef formsignal
  uint8_t bob;
  uint8_t maxbobs;
#endif
};