
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
#include <Ticker.h>

const uint8_t maxCntChannels = 24;
const uint8_t minCntChannels = 8;
uint8_t cntChannels;

// EEPROM-Adressen
#define setup_done 0x47
// EEPROM-Belegung
// EEPROM-Speicherplätze der Local-IDs
const uint16_t adr_setup_done = 0x00;
const uint8_t adr_decoderadr = 0x01;
const uint16_t adr_cntChannels = 0x02;
const uint16_t lastAdr = adr_cntChannels + 1;
#define EEPROM_SIZE lastAdr

#define CAN_FRAME_SIZE 13 /* maximum datagram size */

// config-Daten
// Parameter-Kanäle
enum Kanals
{
  Kanal00,
  Kanal01,
  Kanal02,
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
Variablen der Signals & Magnetartikel
*/
const uint8_t zPin = GPIO_NUM_4;
const uint8_t s3Pin = GPIO_NUM_16;
const uint8_t s2Pin = GPIO_NUM_17;
const uint8_t s1Pin = GPIO_NUM_5;
const uint8_t s0Pin = GPIO_NUM_18;
const uint8_t enablePin = GPIO_NUM_19;
uint8_t pins[] = {enablePin, s0Pin, s1Pin, s2Pin, s3Pin};
const uint8_t channel17 = GPIO_NUM_36;
const uint8_t channel18 = GPIO_NUM_39;
const uint8_t channel19 = GPIO_NUM_34;
const uint8_t channel20 = GPIO_NUM_35;
const uint8_t channel21 = GPIO_NUM_32;
const uint8_t channel22 = GPIO_NUM_33;
const uint8_t channel23 = GPIO_NUM_25;
const uint8_t channel24 = GPIO_NUM_26;
uint8_t channel17to24[] = {channel17, channel18, channel19, channel20,
                           channel21, channel22, channel23, channel24};
const uint8_t isFree = 0;
const uint8_t isOccupied = 1;
uint8_t channels[maxCntChannels];

uint8_t msecs[maxCntChannels];
uint8_t inputValue[maxCntChannels];

// Protokollkonstante
#define PROT MM_ACC

// Forward declaration
void sendCanFrame();
void process_sensor_event(uint8_t channel);

unsigned long time2Poll;

#include "espnow.h"

Ticker tckr0; // each msec
const float tckr0Time = 0.01 / maxCntChannels;

// Routine meldet an die CANguru-Bridge, wenn eine Statusänderung
// eines Blockabschnittes bemerkt wurde
void process_sensor_event(uint8_t channel)
{
  memset(opFrame, 0, sizeof(opFrame));
  opFrame[0x01] = S88_EVENT;
  // Adresse (maximal 255), d.h. 10 Module
  opFrame[0x08] = channel + 1;
  uint8_t chAdr = channel % maxCntChannels;
  // Zustand
  // alt und neu
  if (channels[chAdr] == isOccupied)
  {
    opFrame[0x09] = isFree;
    opFrame[0x0A] = isOccupied;
  }
  else
  {
    opFrame[0x09] = isOccupied;
    opFrame[0x0A] = isFree;
  }
  opFrame[0x04] = 8;
  sendCanFrame();
}

// Auswertung der Melderinformationen
int readPin(uint8_t inputPin)
{
  if (inputPin < 16)
  {
    for (uint8_t bits = 0; bits<4; bits++)
      digitalWrite(pins[bits+1], bitRead(inputPin, bits));
//    delayMicroseconds(1);
    return digitalRead(zPin);
  }
  else
  {
    return digitalRead(channel17to24[inputPin - 16]);
  }
}

// Timer steuert die Abfrage der Melder
void timer1ms()
{
  static uint8_t currChannel = 0;
  if (readPin(currChannel))
  {
    inputValue[currChannel]++;
  }
  msecs[currChannel]++;
  if (msecs[currChannel] > 50)
  {
    if ((inputValue[currChannel] < 45) && (inputValue[currChannel] > 0))
    {
      if (channels[currChannel] != isOccupied)
      {
        // speichern
        channels[currChannel] = isOccupied;
        process_sensor_event(currChannel);
      }
    }
    else
    {
      if (channels[currChannel] != isFree)
      {
        // speichern
        channels[currChannel] = isFree;
        process_sensor_event(currChannel);
      }
    }
    msecs[currChannel] = 0;
    inputValue[currChannel] = 0;
  }
  currChannel++;
  if (currChannel >= cntChannels)
  {
    currChannel = 0;
  }
}

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
  Serial.println("\r\n\r\nR ü c k m e l d e r");
  // der Decoder strahlt mit seiner Kennung
  // damit kennt die CANguru-Bridge (der Master) seine Decoder findet
  startAPMode();
  // der Master wird registriert
  addMaster();
  WiFi.disconnect();
  if (!EEPROM.begin(EEPROM_SIZE))
  {
    Serial.println("failed to initialise EEPROM");
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

    // Anzahl der angeschlossenen Melder
    cntChannels = 18;
    EEPROM.write(adr_cntChannels, cntChannels);
    EEPROM.commit();

    // setup_done auf "TRUE" setzen
    EEPROM.write(adr_setup_done, setup_done);
    EEPROM.commit();
  }
  else
  {
    // nach dem ersten Mal Einlesen der gespeicherten Werte
    // Adresse
    params.decoderadr = readValfromEEPROM(adr_decoderadr, minadr, minadr, maxadr);
    // Anzahl der angeschlossenen Melder
    cntChannels = readValfromEEPROM(adr_cntChannels, 18, minCntChannels, maxCntChannels);
  }
  // ab hier werden die Anweisungen bei jedem Start durchlaufen
  // Flags
  got1CANmsg = false;
  SYS_CMD_Request = false;
  statusPING = false;
  initialData2send = false;
  time2Poll = 0;
  memset(msecs, 0, sizeof(msecs));
  memset(inputValue, 0, sizeof(inputValue));
  memset(channels, isFree, sizeof(channels));
// initialize digital pins as an input / output.
  pinMode(zPin, INPUT);
  for (uint8_t p = 0; p < sizeof(pins); p++)
    pinMode(pins[p], OUTPUT);
  digitalWrite(enablePin, LOW);
  for (uint8_t c = 0; c < sizeof(channel17to24); c++)
    pinMode(channel17to24[c], INPUT);
  // Vorbereiten der Blink-LED
  stillAliveBlinkSetup();
}

// Zu Beginn des Programmablaufs werden die aktuellen Statusmeldungen an WDP geschickt
void sendTheInitialData()
{
  initialData2send = false;
  for (uint8_t ch = 0; ch < cntChannels; ch++)
  {
    process_sensor_event(ch);
    delay(wait_time_medium);
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
  // opFrame[4] ist die Anzahl der gesetzten Datenbytes
  // alle Bytes jenseits davon werden auf 0x00 gesetzt
  for (uint8_t i = CAN_FRAME_SIZE - 1; i < 8 - opFrame[4]; i--)
    opFrame[i] = 0x00;
  // durch Increment wird das Response-Bit gesetzt
  opFrame[1]++;
  // opFrame[2] und opFrame[3] enthalten den Hash, der zu Beginn des Programmes
  // einmal errechnet wird
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
  // Kanalnummer #1 - Decoderadresse
  case 1:
  {
    oldval = params.decoderadr;
    params.decoderadr = (opFrame[11] << 8) + opFrame[12];
    if (testMinMax(oldval, params.decoderadr, minadr, maxadr))
    {
      // speichert die neue Adresse
      EEPROM.write(adr_decoderadr, params.decoderadr);
      EEPROM.commit();
    }
    else
    {
      params.decoderadr = oldval;
    }
  }
  break;
  // Kanalnummer #1 - Anzahl Kanäle / Melder
  case 2:
  {
    oldval = cntChannels;
    cntChannels = (opFrame[11] << 8) + opFrame[12];
    if (testMinMax(oldval, cntChannels, minCntChannels, maxCntChannels))
    {
      // speichert die neue Anzahl
      EEPROM.write(adr_cntChannels, cntChannels);
      EEPROM.commit();
    }
    else
    {
      cntChannels = oldval;
    }
  }
  break;
  }
  // antworten
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
  opFrame[11] = DEVTYPE_RM >> 8;
  opFrame[12] = DEVTYPE_RM;
  sendCanFrame();
  if (time2Poll == 0)
    time2Poll = millis();
}

// auf Anforderung des CANguru-Servers sendet der Decoder
// mit dieser Prozedur sendConfig seine Parameterwerte
void sendConfig()
{
  const uint8_t Kanalwidth = 8;
  const uint8_t numberofKanals = endofKanals - 1;

  const uint8_t NumLinesKanal00 = 5 * Kanalwidth;
  uint8_t arrKanal00[NumLinesKanal00] = {
      /*1*/ Kanal00, numberofKanals, (uint8_t)0, (uint8_t)0, (uint8_t)0, (uint8_t)0, (uint8_t)0, params.decoderadr,
      /*2.1*/ (uint8_t)highbyte2char(hex2dec(params.uid_device[0])), (uint8_t)lowbyte2char(hex2dec(params.uid_device[0])),
      /*2.2*/ (uint8_t)highbyte2char(hex2dec(params.uid_device[1])), (uint8_t)lowbyte2char(hex2dec(params.uid_device[1])),
      /*2.3*/ (uint8_t)highbyte2char(hex2dec(params.uid_device[2])), (uint8_t)lowbyte2char(hex2dec(params.uid_device[2])),
      /*2.4*/ (uint8_t)highbyte2char(hex2dec(params.uid_device[3])), (uint8_t)lowbyte2char(hex2dec(params.uid_device[3])),
      /*3*/ 'C', 'A', 'N', 'g', 'u', 'r', 'u', ' ',
      /*4*/ 'B', 'e', 's', 'e', 't', 'z', 't', '-',
      /*5*/ 'M', 'e', 'l', 'd', 'e', 'r', 0, 0};
  const uint8_t NumLinesKanal01 = 4 * Kanalwidth;
  uint8_t arrKanal01[NumLinesKanal01] = {
      // #2 - WORD immer Big Endian, wie Uhrzeit
      /*1*/ Kanal01, 2, 0, 1, 0, maxadr, 0, params.decoderadr,
      /*2*/ 'M', 'o', 'd', 'u', 'l', 'a', 'd', 'r',
      /*3*/ 'e', 's', 's', 'e', 0, '0', 0, (maxadr / 100) + '0',
      /*4*/ (maxadr - (uint8_t)(maxadr / 100) * 100) / 10 + '0', (maxadr - (uint8_t)(maxadr / 10) * 10) + '0', 0, 'A', 'd', 'r', 0, 0};
  const uint8_t NumLinesKanal02 = 5 * Kanalwidth;
  uint8_t arrKanal02[NumLinesKanal02] = {
      // #2 - WORD immer Big Endian, wie Uhrzeit
      /*1*/ Kanal02, 2, 0, minCntChannels, 0, maxCntChannels, 0, cntChannels,
      /*2*/ 'A', 'n', 'z', 'a', 'h', 'l', ' ', 'M',
      /*3*/ 'e', 'l', 'd', 'e', 'r', 0, '0', 0,
      /*4*/ (maxCntChannels / 100) + '0', (maxCntChannels - (uint8_t)(maxCntChannels / 100) * 100) / 10 + '0', (maxCntChannels - (uint8_t)(maxCntChannels / 10) * 10) + '0', 0, 'C', 'n', 't',
      /*5*/ 0, 0, 0, 0, 0, 0, 0, 0};
  uint8_t NumKanalLines[numberofKanals + 1] = {
      NumLinesKanal00, NumLinesKanal01, NumLinesKanal02};

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
  if (time2Poll > 0)
  {
    // die Melder werden periodisch nach Statusänderungen abgefragt
    if (millis() > (time2Poll + 500))
    {
      tckr0.attach(tckr0Time, timer1ms); // each msec
      time2Poll = 0;
    }
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
