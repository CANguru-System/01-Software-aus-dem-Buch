
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
#include "esp_system.h"
#include <esp_now.h>
#include <esp_wifi.h>
#include <lights.h>
#include <licht.h>
#include <ticker.h>

// LEDArrays
const uint8_t cntChannels = 4;

// EEPROM-Adressen
#define setup_done 0x47
// EEPROM-Belegung
// EEPROM-Speicherplätze der Local-IDs
const uint8_t adr_setup_done = 0x00;
// Kanalnummer #1 - Dunkelphase
const uint16_t adr_offTime = 0x01;
// Kanalnummer #2 - Helligkeit
const uint16_t adr_onTime = 0x03;
// Kanalnummer #3 - Dim
const uint16_t adr_Dim = 0x05;
// Kanalnummer #24- Geschwindigkeit
const uint8_t adr_speed = 0x07;
// Kanalnummer #5 ff- LEDProgramm für die 4 Kanäle festlegen
const uint16_t adr_prog = 0x08; // ab dieser Adresse werden die Weichenstellungen gespeichert
const uint16_t lastAdr = adr_prog;
#define EEPROM_SIZE lastAdr + cntChannels

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
  // Kanal05 ist der erste LEDProgramm-Kanal
  Kanal05,
  Kanal06,
  Kanal07,
  Kanal08,
  endofKanals
};

Kanals CONFIGURATION_Status_Index = Kanal00;

// Zeigen an, ob eine entsprechende Anforderung eingegangen ist
bool CONFIG_Status_Request = false;
bool SYS_CMD_Request = false;

// Timer
boolean statusPING;

#define VERS_HIGH 0x00 // Versionsnummer vor dem Punkt
#define VERS_LOW 0x01  // Versionsnummer nach dem Punkt

Ticker tckr1;
const float tckr1Time = 0.01;

CANguruLight CANguruLight0;
CANguruLight CANguruLight1;
CANguruLight CANguruLight2;
CANguruLight CANguruLight3;

CANguruLight channels[cntChannels] = {CANguruLight0, CANguruLight1, CANguruLight2, CANguruLight3};

// Forward declaration
void sendConfig();

#include "espnow.h"

void timer1ms()
{
  for (uint8_t ch = 0; ch < cntChannels; ch++)
  {
    channels[ch].Update();
  }
}

// Funktion stellt sicher, dass keine unerlaubten Werte geladen werden können
uint16_t readValfromEEPROM(uint16_t adr, uint16_t val, uint16_t min, uint16_t max)
{
  uint16_t v = EEPROM.readUShort(adr);
  if ((v >= min) && (v <= max))
    return v;
  else
    return val;
}

void setup()
{
  Serial.begin(bdrMonitor);
  Serial.println("\r\n\r\nL i c h t d e c o d e r");
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
    // wurde das Setup bereits einmal durchgeführt?
    // dann wird dieser Anteil übersprungen
    // 47, weil das EEPROM (hoffentlich) nie ursprünglich diesen Inhalt hatte

    // Kanalnummer #1 - Dunkelheit
    // setzt die offTime anfangs auf 0
    EEPROM.writeUShort(adr_offTime, offTimeStart);
    EEPROM.commit();

    // Kanalnummer #2 - Helligkeit
    // setzt die onTime anfangs auf 4095
    EEPROM.writeUShort(adr_onTime, onTimeStart);
    EEPROM.commit();

    // Kanalnummer #3 - Dim
    // setzt den Dim-Faktor anfangs auf 16
    EEPROM.writeUShort(adr_Dim, dimStart);
    EEPROM.commit();

    // Kanalnummer #4 - Geschwindigkeit
    // setzt die speed anfangs auf 0
    EEPROM.write(adr_speed, speedStart);
    EEPROM.commit();

    // Kanalnummer #5 ff- LEDProgramm für die 4 Kanäle festlegen
    // Programme auf die ersten vier setzen setzen
    for (uint8_t ch = 0; ch < cntChannels; ch++)
    {
      EEPROM.write(adr_prog + ch, ch);
      EEPROM.commit();
    }
    // setup_done auf "TRUE" setzen
    EEPROM.write(adr_setup_done, setup_done);
    EEPROM.commit();
  }
  // ab hier werden die Anweisungen bei jedem Start durchlaufen
  got1CANmsg = false;
  statusPING = false;
  SYS_CMD_Request = false;
  params.decoderadr = 0;
  for (uint8_t ch = 0; ch < cntChannels; ch++)
  {
  // Kanalnummer #1 - Dunkelphase
    channels[ch].setoffTime(readValfromEEPROM(adr_offTime, offTimeStart, offTimeMin, offTimeMax));
  // Kanalnummer #2 - Helligkeit
    channels[ch].setonTime(readValfromEEPROM(adr_onTime, onTimeStart, onTimeMin, onTimeMax));
  // Kanalnummer #3 - Dim
    channels[ch].setDim(readValfromEEPROM(adr_Dim, dimStart, dimMin, dimMax));
  // Kanalnummer #4 - Geschwindigkeit
    channels[ch].setSpeed(readValfromEEPROM(adr_speed, speedStart, speedMin, speedMax));
  // Kanalnummer #5 ff- LEDProgramm für die einzelnen Expander festlegen
    channels[ch].begin(ch, (LEDPrograms)readValfromEEPROM(adr_prog+ch, ch, progMin, progMax));
  }
  // every milli sec
  tckr1.attach(tckr1Time, timer1ms);
  // Vorbereiten der Blink-LED
  stillAliveBlinkSetup();
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
Hash	16 Bit	Kollisionsauflösung		15 .. 0
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

// Mit testMinMax wird festgestellt, ob ein Wert innerhalb der
// Grenzen von min und max liegt
bool testMinMax(uint16_t oldval, uint16_t val, uint16_t min, uint16_t max)
{
  return (oldval != val) && (val >= min) && (val <= max);
}

// receiveKanalData dient der Parameterübertragung zwischen Decoder und CANguru-Server
// es erhält die evtuelle auf dem Server geänderten Werte zurück
void receiveKanalData()
{
  SYS_CMD_Request = false;
  uint16_t oldval;
  uint16_t newval;
  switch (opFrame[10])
  {
  // Kanalnummer #1 - Dunkelheit
  case 1:
  {
    oldval = EEPROM.readUShort(adr_offTime);
    newval = (opFrame[11] << 8) + opFrame[12];
    if (testMinMax(oldval, newval, offTimeMin, offTimeMax))
    {
      EEPROM.writeUShort(adr_offTime, newval);
      EEPROM.commit();
      for (uint8_t ch = 0; ch < cntChannels; ch++)
      {
        // neue Dunkelheit
        channels[ch].setoffTime(newval);
      }
    }
  }
  break;
  // Kanalnummer #2 - Helligkeit
  case 2:
  {
    oldval = EEPROM.readUShort(adr_onTime);
    newval = (opFrame[11] << 8) + opFrame[12];
    if (testMinMax(oldval, newval, onTimeMin, onTimeMax))
    {
      EEPROM.writeUShort(adr_onTime, newval);
      EEPROM.commit();
      for (uint8_t ch = 0; ch < cntChannels; ch++)
      {
        // neue Helligkeit
        channels[ch].setonTime(newval);
      }
    }
  }
  // Kanalnummer #3 - Dim
  case 3:
  {
    oldval = EEPROM.readUShort(adr_Dim);
    newval = (opFrame[11] << 8) + opFrame[12];
    if (testMinMax(oldval, newval, dimMin, dimMax))
    {
      EEPROM.writeUShort(adr_Dim, newval);
      EEPROM.commit();
      for (uint8_t ch = 0; ch < cntChannels; ch++)
      {
        // neuer Dim-Faktor
        channels[ch].setDim(newval);
      }
    }
  }
  // Kanalnummer #4 - Geschwindigkeit
  case 4:
  {
    oldval = EEPROM.read(adr_speed);
    newval = (opFrame[11] << 8) + opFrame[12];
    if (testMinMax(oldval, newval, speedMin, speedMax))
    {
      EEPROM.write(adr_speed, (uint8_t)(newval));
      EEPROM.commit();
      for (uint8_t ch = 0; ch < cntChannels; ch++)
      {
        // neue Geschwindigkeit
        channels[ch].setSpeed((uint8_t)(newval));
      }
    }
  }
  break;
  // Kanalnummer #5 ff- LEDProgramm für die 4 Kanäle setzen
  case 5:
  case 6:
  case 7:
  case 8:
  {
    uint8_t ch = opFrame[10] - Kanal05;
    oldval = EEPROM.read(adr_prog + ch);
    newval = opFrame[12];
    if (testMinMax(oldval, newval, progMin, progMax))
    {
      EEPROM.write(adr_prog + ch, newval);
      EEPROM.commit();
      channels[ch].setPrg((LEDPrograms)newval);
    }
    Serial.println();
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
  opFrame[11] = DEVTYPE_LIGHT >> 8;
  opFrame[12] = DEVTYPE_LIGHT;
  sendCanFrame();
}

// auf Anforderung des CANguru-Servers sendet der Decoder
// mit dieser Prozedur sendConfig seine Parameterwerte
void sendConfig()
{
  const uint8_t Kanalwidth = 8;
  const uint8_t numberofKanals = endofKanals - 1;

  const uint8_t NumLinesKanal00 = 5 * Kanalwidth;
  uint8_t arrKanal00[NumLinesKanal00] = {
      /*1*/ Kanal00, numberofKanals, 0, 0, 0, 0, 0, params.decoderadr,
      /*2.1*/ (uint8_t)highbyte2char(hex2dec(params.uid_device[0])), (uint8_t)lowbyte2char(hex2dec(params.uid_device[0])),
      /*2.2*/ (uint8_t)highbyte2char(hex2dec(params.uid_device[1])), (uint8_t)lowbyte2char(hex2dec(params.uid_device[1])),
      /*2.3*/ (uint8_t)highbyte2char(hex2dec(params.uid_device[2])), (uint8_t)lowbyte2char(hex2dec(params.uid_device[2])),
      /*2.4*/ (uint8_t)highbyte2char(hex2dec(params.uid_device[3])), (uint8_t)lowbyte2char(hex2dec(params.uid_device[3])),
      /*3*/ 'C', 'A', 'N', 'g', 'u', 'r', 'u', ' ',
      /*4*/ 'L', 'i', 'c', 'h', 't', 'd', 'e', 'c',
      /*5*/ 'o', 'd', 'e', 'r', 0, 0, 0, 0};
  // Kanalnummer #1 - Dunkelkeit
  const uint8_t NumLinesKanal01 = 4 * Kanalwidth;
  uint8_t arrKanal01[NumLinesKanal01] = {
      /*1*/ Kanal01, 2, 0, 0, 0x0F, 0xFF, EEPROM.read(adr_offTime + 1), EEPROM.read(adr_offTime),
      /*2*/ 'D', 'u', 'n', 'k', 'e', 'l', 'p', 'h',
      /*3*/ 'a', 's', 'e', 0, '0', 0, '4', '0',
      /*4*/ '9', '5', 0, 'D', 'i', 'm', 0, 0};
  // Kanalnummer #2 - Helligkeit
  const uint8_t NumLinesKanal02 = 4 * Kanalwidth;
  uint8_t arrKanal02[NumLinesKanal02] = {
      /*1*/ Kanal02, 2, 0, 0, 0x0F, 0xFF, EEPROM.read(adr_onTime + 1), EEPROM.read(adr_onTime),
      /*2*/ 'H', 'e', 'l', 'l', 'p', 'h', 'a', 's',
      /*3*/ 'e', 0, '0', 0, '4', '0', '9', '5',
      /*4*/ 0, 'D', 'i', 'm', 0, 0, 0, 0};
  // Kanalnummer #3 - Dim
  const uint8_t NumLinesKanal03 = 4 * Kanalwidth;
  uint8_t arrKanal03[NumLinesKanal03] = {
      /*1*/ Kanal03, 2, 0, 8, 0x04, 0x00, EEPROM.read(adr_Dim + 1), EEPROM.read(adr_Dim),
      /*2*/ 'D', 'i', 'm', '-', 'F', 'a', 'k', 't',
      /*3*/ 'o', 'r', 0, '0', 0, '1', '0', '2',
      /*4*/ '4', 0, 'D', 'i', 'm', 0, 0, 0};
  // Kanalnummer #4 - Geschwindigkeit
  const uint8_t NumLinesKanal04 = 4 * Kanalwidth;
  uint8_t arrKanal04[NumLinesKanal04] = {
      /*1*/ Kanal04, 2, 0, 0, 0, 20, 0, EEPROM.read(adr_speed),
      /*2*/ 'G', 'e', 's', 'c', 'h', 'w', 'i', 'n',
      /*3*/ 'd', 'i', 'g', 'k', 'e', 'i', 't', 0,
      /*4*/ '0', 0, '2', '0', 0, 'm', 's', 0};
  // Kanalnummer #5 ff- LEDProgramm für die einzelnen Expander festlegen
  const uint8_t NumLinesKanal05 = 10 * Kanalwidth;
  uint8_t arrKanal05[NumLinesKanal05] = {
      /*01*/ CONFIGURATION_Status_Index, 1, numberofLEDProgs, EEPROM.read((uint8_t)adr_prog + CONFIGURATION_Status_Index - Kanal05), 0, 0, 0, 0,
      /*02*/ 'K', 'a', 'n', 'a', 'l', ' ', (uint8_t)(CONFIGURATION_Status_Index - Kanal05 + 1 + '0'), 0,
      /*03*/ 'K', 'n', 'i', 'g', 'h', 't', 'R', 'i',
      /*04*/ 'd', 'e', 'r', 0, 'B', 'a', 'u', 's',
      /*05*/ 't', 'e', 'l', 'l', 'e', 0, 'A', 'm',
      /*06*/ 'p', 'e', 'l', 0, 'B', 'l', 'a', 'u',
      /*07*/ 'l', 'i', 'c', 'h', 't', 0, 'H', 'a',
      /*08*/ 'u', 's', '1', 0, 'H', 'a', 'u', 's',
      /*09*/ '2', 0, 'H', 'a', 'u', 's', '3', 0,
      /*10*/ 'H', 'a', 'u', 's', '4', 0, 0, 0};

  uint8_t NumKanalLines[numberofKanals + 1] = {
      NumLinesKanal00, NumLinesKanal01, NumLinesKanal02, NumLinesKanal03, NumLinesKanal04,
      // die LEDProgramm-Kanäle haben alle die gleiche Länge
      NumLinesKanal05, NumLinesKanal05, NumLinesKanal05, NumLinesKanal05};

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
    case Kanal06:
    case Kanal07:
    case Kanal08:
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
  for (uint8_t ch = 0; ch < cntChannels; ch++)
  {
    channels[ch].switchLeds();
  }
  if (got1CANmsg)
  {
    got1CANmsg = false;
    // auf PING antworten
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
  }
}
