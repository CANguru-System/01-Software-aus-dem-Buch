
/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <CANguru-Buch@web.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gustav Wostrack
 * ----------------------------------------------------------------------------
 */

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include "CANguruDefs.h"
#include "EEPROM.h"
#include "esp32-hal-ledc.h"
#include "Sweeper.h"
#include "esp_system.h"
#include <esp_now.h>
#include <esp_wifi.h>
#include <Ticker.h>
#include "device2use.h"

// Anzahl der Magnetartikel
#define num_servos 4

// EEPROM-Adressen
#define setup_done 0x47
// EEPROM-Belegung
// EEPROM-Speicherplätze der Local-IDs
const uint16_t adr_setup_done = 0x00;
const uint16_t adr_decoderadr = 0x01;
const uint16_t adr_SrvDel = 0x02;
const uint16_t adr_angleStart = 0x03;
const uint16_t adr_angleStopp = 0x04;
const uint16_t adr_currEnd = 0x05;
const uint16_t acc_state = 0x06; // ab dieser Adresse werden die Weichenstellungen gespeichert
const uint16_t lastAdr = acc_state + num_servos;
const uint16_t EEPROM_SIZE = lastAdr;

#define CAN_FRAME_SIZE 13 /* maximum datagram size */

// config-Daten
// Parameter-Kanäle
enum Kanals
{
  Kanal00,
  Kanal01,
  Kanal02,
  Kanal03,
  Kanal04,
  Kanal05,
  endofKanals
};

Kanals CONFIGURATION_Status_Index = Kanal00;

// Zeigen an, ob eine entsprechende Anforderung eingegangen ist
bool CONFIG_Status_Request = false;
bool SYS_CMD_Request = false;

// Timer
boolean statusPING;
boolean initialData2send;

#define VERS_HIGH 0x00 // Versionsnummer vor dem Punkt
#define VERS_LOW 0x01  // Versionsnummer nach dem Punkt

/*
Variablen der Servos & Magnetartikel
*/
Sweeper servo00;
Sweeper servo01;
Sweeper servo02;
Sweeper servo03;

Sweeper Servos[num_servos] = {servo00, servo01, servo02, servo03};
uint8_t servoDelay;

// an diese PINs werden die Magnetartikel angeschlossen
uint8_t acc_pin_outs[num_servos] = {GPIO_NUM_4, GPIO_NUM_16, GPIO_NUM_17, GPIO_NUM_5}; //PIN-Zuordnung

uint8_t servoAngleStart;
uint8_t servoAngleStopp;
uint8_t currEnd;
const int16_t minAngle = 1;
const int16_t maxAngle = 179;
const int16_t minEnd = 1;
const int16_t maxEnd = 25;

// Protokollkonstante
#define PROT MM_ACC

// Forward declaration
void switchAcc(uint8_t acc_num);
void acc_report(uint8_t num);
void calc_to_address();
void sendConfig();
void generateHash(uint8_t offset);

#include "espnow.h"

// Funktion stellt sicher, dass keine unerlaubten Werte geladen werden können
uint8_t readValfromEEPROM(uint16_t adr, uint8_t val, uint8_t min, uint8_t max)
{
  uint8_t v = EEPROM.read(adr);
  if ((v >= min) && (v <= max))
    return v;
  else
    return val;
}

void setup()
{
  Serial.begin(bdrMonitor);
#ifdef armservo
  Serial.println("\r\n\r\nCANguru - Weiche");
#endif
#ifdef linearservo
  Serial.println("\r\n\r\nCANguru - Weiche - Linear-Servo");
#endif
#ifdef formsignal
  Serial.println("\r\n\r\nCANguru - Formsignal");
#endif
  // der Decoder strahlt mit seiner Kennung
  // damit kennt die CANguru-Bridge (der Master) seine Decoder findet
  startAPMode();
  // der Master (CANguru-Bridge) wird registriert
  addMaster();
  // WLAN -Verbindungen können wieder ausgeschaltet werden
  WiFi.disconnect();
  // die EEPROM-Library wird gestartet
  if (!EEPROM.begin(EEPROM_SIZE))
  {
    Serial.println("Failed to initialise EEPROM");
  }
  uint8_t setup_todo = EEPROM.read(adr_setup_done);
  if (setup_todo != setup_done)
  {
    // alles fürs erste Mal
    //
    // wurde das Setup bereits einmal durchgeführt?
    // dann wird dieser Anteil übersprungen
    // 47, weil das EEPROM (hoffentlich) nie ursprünglich diesen Inhalt hatte

    // setzt die Boardnum anfangs auf 1
    params.decoderadr = 1;
    EEPROM.write(adr_decoderadr, params.decoderadr);
    EEPROM.commit();
    // Festlegen der Winkel
#ifdef armservo
    servoAngleStart = 5;
#endif
#ifdef linearservo
    servoAngleStart = 15;
#endif
#ifdef formsignal
    servoAngleStart = 20;
#endif
    EEPROM.write(adr_angleStart, servoAngleStart);
    EEPROM.commit();
#ifdef armservo
    servoAngleStopp = 74;
#endif
#ifdef linearservo
    servoAngleStopp = 115;
#endif
#ifdef formsignal
    servoAngleStopp = 50;
#endif
    EEPROM.write(adr_angleStopp, servoAngleStopp);
    EEPROM.commit();
    currEnd = 5;
    EEPROM.write(adr_currEnd, currEnd);
    EEPROM.commit();
    // Verzögerung
#ifdef armservo
    EEPROM.write(adr_SrvDel, stdservodelay);
#endif
#ifdef linearservo
    EEPROM.write(adr_SrvDel, stdservodelay * 3);
#endif
#ifdef formsignal
    EEPROM.write(adr_SrvDel, stdservodelay);
#endif
    EEPROM.commit();
    // Status der Magnetartikel zu Beginn auf rechts setzen
    for (uint8_t servo = 0; servo < num_servos; servo++)
    {
      EEPROM.write(acc_state + servo, right);
      EEPROM.commit();
    }
    // setup_done auf "TRUE" setzen
    EEPROM.write(adr_setup_done, setup_done);
    EEPROM.commit();
  }
  else
  {
    // nach dem ersten Mal Einlesen der gespeicherten Werte
    // Adresse
    params.decoderadr = readValfromEEPROM(adr_decoderadr, minadr, minadr, maxadr);
    // Winkel
#ifdef armservo
    servoAngleStart = readValfromEEPROM(adr_angleStart, 5, minAngle, maxAngle);
#endif
#ifdef linearservo
    servoAngleStart = readValfromEEPROM(adr_angleStart, 15, minAngle, maxAngle);
#endif
#ifdef formsignal
    servoAngleStart = readValfromEEPROM(adr_angleStart, 20, minAngle, maxAngle);
#endif
#ifdef armservo
    servoAngleStopp = readValfromEEPROM(adr_angleStopp, 74, minAngle, maxAngle);
#endif
#ifdef linearservo
    servoAngleStopp = readValfromEEPROM(adr_angleStopp, 115, minAngle, maxAngle);
#endif
#ifdef formsignal
    servoAngleStopp = readValfromEEPROM(adr_angleStopp, 50, minAngle, maxAngle);
#endif
#ifdef armservo
    servoAngleStopp = readValfromEEPROM(adr_angleStopp, 74, minAngle, maxAngle);
#endif
#ifdef linearservo
    servoAngleStopp = readValfromEEPROM(adr_angleStopp, 115, minAngle, maxAngle);
#endif
#ifdef formsignal
    servoAngleStopp = readValfromEEPROM(adr_angleStopp, 50, minAngle, maxAngle);
#endif
    currEnd = readValfromEEPROM(adr_currEnd, 5, minEnd, maxEnd);
  }
  // ab hier werden die Anweisungen bei jedem Start durchlaufen
  // Flags
  got1CANmsg = false;
  SYS_CMD_Request = false;
  statusPING = false;
  initialData2send = false;
  // Variablen werden gemäß der eingelsenen Werte gesetzt
  // evtl. werden auch die Servos verändert
#ifdef armservo
    servoDelay = readValfromEEPROM(adr_SrvDel, stdservodelay, minservodelay, maxservodelay);
#endif
#ifdef linearservo
    servoDelay = readValfromEEPROM(adr_SrvDel, 3 * stdservodelay, minservodelay, maxservodelay);
#endif
#ifdef formsignal
    servoDelay = readValfromEEPROM(adr_SrvDel, stdservodelay, minservodelay, maxservodelay);
#endif
  for (uint8_t servo = 0; servo < num_servos; servo++)
  {
    // Status der Magnetartikel versenden an die Servos
    Servos[servo].SetPosCurr((position)EEPROM.read(acc_state + servo));
    // Servos mit den PINs verbinden, initialisieren & Artikel setzen wie gespeichert
    Servos[servo].SetDelay(servoDelay);
    Servos[servo].SetAngle(currEnd, servoAngleStart, servoAngleStopp);
    Servos[servo].Attach(acc_pin_outs[servo], servo);
    Servos[servo].SetPosition();
  }
  // berechnet die _to_address aus der Adresse und der Protokollkonstante
  calc_to_address();
  // Vorbereiten der Blink-LED
  stillAliveBlinkSetup();
}

// Zu Beginn des Programmablaufs werden die aktuellen Statusmeldungen an WDP geschickt
void sendTheInitialData()
{
  initialData2send = false;
  for (uint8_t servo = 0; servo < num_servos; servo++)
  {
    // Status der Magnetartikel einlesen in lokale arrays
    acc_report(servo);
  }
}

/*
Response
Bestimmt, ob CAN Meldung eine Anforderung oder Antwort oder einer
vorhergehende Anforderung ist. Grundsätzlich wird eine Anforderung
ohne ein gesetztes Response Bit angestoßen. Sobald ein Kommando
ausgeführt wurde, wird es mit gesetztem Response Bit, sowie dem
ursprünglichen Meldungsinhalt oder den angefragten Werten, bestätigt.
Jeder Teilnehmer am Bus, welche die Meldung ausgeführt hat, bestätigt ein
Kommando.
*/
void sendCanFrame()
{
  // to Server
  for (uint8_t i = CAN_FRAME_SIZE - 1; i < 8 - opFrame[4]; i--)
    opFrame[i] = 0x00;
  opFrame[1]++;
  opFrame[2] = hasharr[0];
  opFrame[3] = hasharr[1];
  sendTheData();
}

/*
CAN Grundformat
Das CAN Protokoll schreibt vor, dass Meldungen mit einer 29 Bit Meldungskennung,
4 Bit Meldungslänge sowie bis zu 8 Datenbyte bestehen.
Die Meldungskennung wird aufgeteilt in die Unterbereiche Priorit�t (Prio),
Kommando (Command), Response und Hash.
Die Kommunikation basiert auf folgendem Datenformat:

Meldungskennung
Prio	2+2 Bit Message Prio			28 .. 25
Command	8 Bit	Kommando Kennzeichnung	24 .. 17
Resp.	1 Bit	CMD / Resp.				16
Hash	16 Bit	Kollisionsaufl�sung		15 .. 0
DLC
DLC		4 Bit	Anz. Datenbytes
Byte 0	D-Byte 0	8 Bit Daten
Byte 1	D-Byte 1	8 Bit Daten
Byte 2	D-Byte 2	8 Bit Daten
Byte 3	D-Byte 3	8 Bit Daten
Byte 4	D-Byte 4	8 Bit Daten
Byte 5	D-Byte 5	8 Bit Daten
Byte 6	D-Byte 6	8 Bit Daten
Byte 7	D-Byte 7	8 Bit Daten
*/

// Die Adressen der einzelnen Servos werden berechnet,
// dabei wird die Decoderadresse einbezogen, so dass
// das erste Servo die Adresse
// Protokollkonstante (0x3000) + (Decoderadresse-1) * Anzahl Servos pro Decoder
// erhält. Das zweite Servo hat dann die Adresse Servo1 plus 1; die
// folgende analog
// das Kürzel to an verschiedenen Stellen steht für den englischen Begriff
// für Weiche, nämlich turnout
void calc_to_address()
{
  uint16_t baseaddress = (params.decoderadr - 1) * num_servos;
  for (uint8_t servo = 0; servo < num_servos; servo++)
  {
    uint16_t to_address = PROT + baseaddress + servo;
    // _to_addresss einlesen in lokales array
    Servos[servo].Set_to_address(to_address);
  }
}

// Mit testMinMax wird festgestellt, ob ein Wert innerhalb der
// Grenzen von min und max liegt
bool testMinMax(uint8_t oldval, uint8_t val, uint8_t min, uint8_t max)
{
  return (oldval != val) && (val >= min) && (val <= max);
}

// receiveKanalData dient der Parameterübertragung zwischen Decoder und CANguru-Server
// es erhält die evtuelle auf dem Server geänderten Werte zurück
void receiveKanalData()
{
  SYS_CMD_Request = false;
  uint8_t oldval;
  switch (opFrame[10])
  {
  // Kanalnummer #1 - Servoverzögerung
  case 1:
  {
    oldval = servoDelay;
    servoDelay = (opFrame[11] << 8) + opFrame[12];
    if (testMinMax(oldval, servoDelay, minservodelay, maxservodelay))
    {
      EEPROM.write(adr_SrvDel, servoDelay);
      EEPROM.commit();
      for (int servo = 0; servo < num_servos; servo++)
      {
        // neue Verzögerung
        Servos[servo].SetDelay(servoDelay);
      }
    }
    else
    {
      servoDelay = oldval;
    }
  }
  break;
  // Kanalnummer #2 - Decoderadresse
  case 2:
  {
    oldval = params.decoderadr;
    params.decoderadr = (opFrame[11] << 8) + opFrame[12];
    if (testMinMax(oldval, params.decoderadr, minadr, maxadr))
    {
      // speichert die neue Adresse
      EEPROM.write(adr_decoderadr, params.decoderadr);
      EEPROM.commit();
      // neue Adressen
      calc_to_address();
    }
    else
    {
      params.decoderadr = oldval;
    }
  }
  break;
  // Kanalnummer #3 - Servowinkel Start / Anfang
  case 3:
  {
    oldval = servoAngleStart;
    servoAngleStart = (opFrame[11] << 8) + opFrame[12];
    if (testMinMax(oldval, servoAngleStart, minAngle, maxAngle))
    {
      // speichert die neue Adresse
      EEPROM.write(adr_angleStart, servoAngleStart);
      EEPROM.commit();
      for (int servo = 0; servo < num_servos; servo++)
      {
        // neue Winkel
        Servos[servo].SetAngle(currEnd, servoAngleStart, servoAngleStopp);
      }
    }
    else
    {
      servoAngleStart = oldval;
    }
  }
  break;
  // Kanalnummer #4 - Servowinkel Stopp / Ende
  case 4:
  {
    oldval = servoAngleStopp;
    servoAngleStopp = (opFrame[11] << 8) + opFrame[12];
    if (testMinMax(oldval, servoAngleStopp, minAngle, maxAngle))
    {
      // speichert die neue Adresse
      EEPROM.write(adr_angleStopp, servoAngleStopp);
      EEPROM.commit();
      for (int servo = 0; servo < num_servos; servo++)
      {
        // neue Winkel
        Servos[servo].SetAngle(currEnd, servoAngleStart, servoAngleStopp);
      }
    }
    else
    {
      servoAngleStopp = oldval;
    }
  }
  break;
  // Kanalnummer #5 - Ausladungswinkel
  case 5:
  {
    oldval = currEnd;
    currEnd = (opFrame[11] << 8) + opFrame[12];
    if (testMinMax(oldval, currEnd, minEnd, maxEnd))
    {
      // speichert die neue Adresse
      EEPROM.write(adr_currEnd, currEnd);
      EEPROM.commit();
      for (int servo = 0; servo < num_servos; servo++)
      {
        // neue Winkel
        Servos[servo].SetAngle(currEnd, servoAngleStart, servoAngleStopp);
      }
    }
    else
    {
      currEnd = oldval;
    }
  }
  break;
  }
  //
  opFrame[11] = 0x01;
  opFrame[4] = 0x07;
  sendCanFrame();
}

// sendPING ist die Antwort der Decoder auf eine PING-Anfrage
void sendPING()
{
  statusPING = false;
  opFrame[1] = PING;
  opFrame[4] = 0x08;
  for (uint8_t i = 0; i < uid_num; i++)
  {
    opFrame[i + 5] = params.uid_device[i];
  }
  opFrame[9] = VERS_HIGH;
  opFrame[10] = VERS_LOW;
  opFrame[11] = DEVTYPE_SERVO >> 8;
  opFrame[12] = DEVTYPE_SERVO;
  sendCanFrame();
}

// Routine meldet an die CANguru-Bridge, wenn eine Statusänderung
// einer Weiche/Signal eingetreten ist
void acc_report(uint8_t num)
{
  opFrame[1] = SWITCH_ACC;
  opFrame[4] = 0x05;
  // Weichenadresse
  opFrame[5] = 0x00;
  opFrame[6] = 0x00;
  opFrame[7] = (uint8_t)(Servos[num].Get_to_address() >> 8);
  opFrame[8] = (uint8_t)Servos[num].Get_to_address();
  // Meldung der Lage
  opFrame[9] = Servos[num].GetPosCurr();
  sendCanFrame();
  delay(wait_time); // Delay added just so we can have time to open up
}

// Diese Routine leitet den Positionswechsel einer Weiche/Signal ein.
void switchAcc(uint8_t acc_num)
{
  position set_pos = Servos[acc_num].GetPosDest();
  switch (set_pos)
  {
  case left:
  {
    Servos[acc_num].GoLeft();
  }
  break;
  case right:
  {
    Servos[acc_num].GoRight();
  }
  break;
  }
  Servos[acc_num].SetPosCurr(set_pos);
  EEPROM.write(acc_state + acc_num, (uint8_t)set_pos);
  EEPROM.commit();
  acc_report(acc_num);
}

// auf Anforderung des CANguru-Servers sendet der Decoder
// mit dieser Prozedur sendConfig seine Parameterwerte
void sendConfig()
{
  const uint8_t Kanalwidth = 8;
  const uint8_t numberofKanals = endofKanals - 1;
#ifdef armservo
  const uint8_t NumLinesKanal00 = 4 * Kanalwidth;
  uint8_t arrKanal00[NumLinesKanal00] = {
      /*1*/ Kanal00, numberofKanals, (uint8_t)0, (uint8_t)0, (uint8_t)0, (uint8_t)0, (uint8_t)0, params.decoderadr,
      /*2.1*/ (uint8_t)highbyte2char(hex2dec(params.uid_device[0])), (uint8_t)lowbyte2char(hex2dec(params.uid_device[0])),
      /*2.2*/ (uint8_t)highbyte2char(hex2dec(params.uid_device[1])), (uint8_t)lowbyte2char(hex2dec(params.uid_device[1])),
      /*2.3*/ (uint8_t)highbyte2char(hex2dec(params.uid_device[2])), (uint8_t)lowbyte2char(hex2dec(params.uid_device[2])),
      /*2.4*/ (uint8_t)highbyte2char(hex2dec(params.uid_device[3])), (uint8_t)lowbyte2char(hex2dec(params.uid_device[3])),
      /*3*/ 'C', 'A', 'N', 'g', 'u', 'r', 'u', ' ',
      /*4*/ 'S', 'e', 'r', 'v', 'o', (uint8_t)0, (uint8_t)0, (uint8_t)0};
#endif
#ifdef linearservo
  const uint8_t NumLinesKanal00 = 5 * Kanalwidth;
  uint8_t arrKanal00[NumLinesKanal00] = {
      /*1*/ Kanal00, numberofKanals, (uint8_t)0, (uint8_t)0, (uint8_t)0, (uint8_t)0, (uint8_t)0, params.decoderadr,
      /*2.1*/ (uint8_t)highbyte2char(hex2dec(params.uid_device[0])), (uint8_t)lowbyte2char(hex2dec(params.uid_device[0])),
      /*2.2*/ (uint8_t)highbyte2char(hex2dec(params.uid_device[1])), (uint8_t)lowbyte2char(hex2dec(params.uid_device[1])),
      /*2.3*/ (uint8_t)highbyte2char(hex2dec(params.uid_device[2])), (uint8_t)lowbyte2char(hex2dec(params.uid_device[2])),
      /*2.4*/ (uint8_t)highbyte2char(hex2dec(params.uid_device[3])), (uint8_t)lowbyte2char(hex2dec(params.uid_device[3])),
      /*3*/ 'C', 'A', 'N', 'g', 'u', 'r', 'u', ' ',
      /*4*/ 'L', 'i', 'n', 'e', 'a', 'r', '-', 'S',
      /*5*/ 'e', 'r', 'v', 'o', (uint8_t)0, (uint8_t)0, (uint8_t)0, (uint8_t)0};
#endif
#ifdef formsignal
  const uint8_t NumLinesKanal00 = 5 * Kanalwidth;
  uint8_t arrKanal00[NumLinesKanal00] = {
      /*1*/ Kanal00, numberofKanals, (uint8_t)0, (uint8_t)0, (uint8_t)0, (uint8_t)0, (uint8_t)0, params.decoderadr,
      /*2.1*/ (uint8_t)highbyte2char(hex2dec(params.uid_device[0])), (uint8_t)lowbyte2char(hex2dec(params.uid_device[0])),
      /*2.2*/ (uint8_t)highbyte2char(hex2dec(params.uid_device[1])), (uint8_t)lowbyte2char(hex2dec(params.uid_device[1])),
      /*2.3*/ (uint8_t)highbyte2char(hex2dec(params.uid_device[2])), (uint8_t)lowbyte2char(hex2dec(params.uid_device[2])),
      /*2.4*/ (uint8_t)highbyte2char(hex2dec(params.uid_device[3])), (uint8_t)lowbyte2char(hex2dec(params.uid_device[3])),
      /*3*/ 'C', 'A', 'N', 'g', 'u', 'r', 'u', ' ',
      /*4*/ 'F', 'o', 'r', 'm', 's', 'i', 'g', 'n',
      /*5*/ 'a', 'l', 0, 0, 0, 0, 0, 0};
#endif
  const uint8_t NumLinesKanal01 = 5 * Kanalwidth;
  uint8_t arrKanal01[NumLinesKanal01] = {
      /*1*/ Kanal01, 2, 0, minservodelay, 0, maxservodelay, 0, servoDelay,
      /*2*/ 'S', 'e', 'r', 'v', 'o', 'v', 'e', 'r',
      /*3*/ 'z', '\xF6', 'g', 'e', 'r', 'u', 'n', 'g',
      /*4*/ 0, minservodelay + '0', 0, (maxservodelay / 100) + '0', (maxservodelay - (uint8_t)(maxservodelay / 100) * 100) / 10 + '0', (maxservodelay - (uint8_t)(maxservodelay / 10) * 10) + '0', 0, 'm',
      /*5*/ 's', 0, 0, 0, 0, 0, 0, 0};
  const uint8_t NumLinesKanal02 = 4 * Kanalwidth;
  uint8_t arrKanal02[NumLinesKanal02] = {
      // #2 - WORD immer Big Endian, wie Uhrzeit
      /*1*/ Kanal02, 2, 0, minadr, 0, maxadr, 0, params.decoderadr,
      /*2*/ 'M', 'o', 'd', 'u', 'l', 'a', 'd', 'r',
      /*3*/ 'e', 's', 's', 'e', 0, '1', 0, (maxadr / 100) + '0',
      /*4*/ (maxadr - (uint8_t)(maxadr / 100) * 100) / 10 + '0', (maxadr - (uint8_t)(maxadr / 10) * 10) + '0', 0, 'A', 'd', 'r', 0, 0};
  const uint8_t NumLinesKanal03 = 4 * Kanalwidth;
  uint8_t arrKanal03[NumLinesKanal03] = {
      // #2 - WORD immer Big Endian, wie Uhrzeit
      /*1*/ Kanal03, 2, 0, minAngle, 0, maxAngle, 0, servoAngleStart,
      /*2*/ 'S', 'e', 'r', 'v', 'o', 'w', 'i', 'n',
      /*3*/ 'k', 'e', 'l', '1', 0, '1', 0, (uint8_t)(maxAngle / 100) + '0',
      /*4*/ (maxAngle - (uint8_t)(maxAngle / 100) * 100) / 10 + '0', (maxAngle - (uint8_t)(maxAngle / 10) * 10) + '0', 0, 'G', 'r', 'a', 'd', 0};
  const uint8_t NumLinesKanal04 = 4 * Kanalwidth;
  uint8_t arrKanal04[NumLinesKanal04] = {
      // #2 - WORD immer Big Endian, wie Uhrzeit
      /*1*/ Kanal04, 2, 0, minAngle, 0, maxAngle, 0, servoAngleStopp,
      /*2*/ 'S', 'e', 'r', 'v', 'o', 'w', 'i', 'n',
      /*3*/ 'k', 'e', 'l', '2', 0, '1', 0, (uint8_t)(maxAngle / 100) + '0',
      /*4*/ (maxAngle - (uint8_t)(maxAngle / 100) * 100) / 10 + '0', (maxAngle - (uint8_t)(maxAngle / 10) * 10) + '0', 0, 'G', 'r', 'a', 'd', 0};
  const uint8_t NumLinesKanal05 = 4 * Kanalwidth;
  uint8_t arrKanal05[NumLinesKanal05] = {
      // #2 - WORD immer Big Endian, wie Uhrzeit
      /*1*/ Kanal05, 2, 0, minEnd, 0, maxEnd, 0, currEnd,
      /*2*/ 'A', 'u', 's', 'l', 'a', 'd', 'u', 'n',
      /*3*/ 'g', 0, '1', 0, (uint8_t)(maxEnd / 100) + '0', (maxEnd - (uint8_t)(maxEnd / 100) * 100) / 10 + '0', (maxEnd - (uint8_t)(maxEnd / 10) * 10) + '0', 0,
      /*4*/ 'G', 'r', 'a', 'd', 0, 0, 0, 0};
  uint8_t NumKanalLines[numberofKanals + 1] = {
      NumLinesKanal00, NumLinesKanal01, NumLinesKanal02, NumLinesKanal03, NumLinesKanal04, NumLinesKanal05};

  uint8_t paket = 0;
  uint8_t outzeichen = 0;
  CONFIG_Status_Request = false;
  for (uint8_t inzeichen = 0; inzeichen < NumKanalLines[CONFIGURATION_Status_Index]; inzeichen++)
  {
    opFrame[1] = CONFIG_Status + 1;
    switch (CONFIGURATION_Status_Index)
    {
    case Kanal00:
    {
      opFrame[outzeichen + 5] = arrKanal00[inzeichen];
    }
    break;
    case Kanal01:
    {
      opFrame[outzeichen + 5] = arrKanal01[inzeichen];
    }
    break;
    case Kanal02:
    {
      opFrame[outzeichen + 5] = arrKanal02[inzeichen];
    }
    break;
    case Kanal03:
    {
      opFrame[outzeichen + 5] = arrKanal03[inzeichen];
    }
    break;
    case Kanal04:
    {
      opFrame[outzeichen + 5] = arrKanal04[inzeichen];
    }
    break;
    case Kanal05:
    {
      opFrame[outzeichen + 5] = arrKanal05[inzeichen];
    }
    break;
    case endofKanals:
    {
      // der Vollständigkeit geschuldet
    }
    break;
    }
    outzeichen++;
    if (outzeichen == 8)
    {
      opFrame[4] = 8;
      outzeichen = 0;
      paket++;
      opFrame[2] = 0x00;
      opFrame[3] = paket;
      sendTheData();
      delay(wait_time_small);
    }
  }
  //
  memset(opFrame, 0, sizeof(opFrame));
  opFrame[1] = CONFIG_Status + 1;
  opFrame[2] = hasharr[0];
  opFrame[3] = hasharr[1];
  opFrame[4] = 0x06;
  for (uint8_t i = 0; i < 4; i++)
  {
    opFrame[i + 5] = params.uid_device[i];
  }
  opFrame[9] = CONFIGURATION_Status_Index;
  opFrame[10] = paket;
  sendTheData();
  delay(wait_time_small);
}

// In dieser Schleife verbringt der Decoder die meiste Zeit
void loop()
{
  // die boolsche Variable got1CANmsg zeigt an, ob vom Master
  // eine Nachricht gekommen ist; der Zustand der Flags
  // entscheidet dann, welche Routine anschließend aufgerufen wird
  for (uint8_t servo = 0; servo < num_servos; servo++)
  {
    // die Servos werden permant abgefragt, ob es ein Delta zwischen
    // tatsächlicher und gewünschter Servostellung gibt
    Servos[servo].Update();
  }
  if (got1CANmsg)
  {
    got1CANmsg = false;
    // auf PING Antworten
    if (statusPING)
    {
      sendPING();
    }
    // Parameterwerte vom CANguru-Server erhalten
    if (SYS_CMD_Request)
    {
      receiveKanalData();
    }
    // Parameterwerte an den CANguru-Server liefern
    if (CONFIG_Status_Request)
    {
      sendConfig();
    }
    // beim ersten Durchlauf werden Initialdaten an die
    // CANguru-Bridge gesendet
    if (initialData2send)
    {
      sendTheInitialData();
    }
  }
}
