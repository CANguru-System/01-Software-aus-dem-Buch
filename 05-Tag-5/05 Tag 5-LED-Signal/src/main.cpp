
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
#include "PWM.h"
#include "esp_system.h"
#include <esp_now.h>
#include <esp_wifi.h>
#include <Ticker.h>

// Anzahl der Magnetartikel
#define num_signals 4

// EEPROM-Adressen
#define setup_done 0x47
// EEPROM-Belegung
// EEPROM-Speicherplätze der Local-IDs
const uint16_t adr_setup_done = 0x00;
const uint8_t adr_decoderadr = 0x01;
const uint16_t adr_SrvDel = 0x02;
const uint16_t acc_state = 0x03; // ab dieser Adresse werden die Weichenstellungen gespeichert
const uint16_t lastAdr = acc_state + num_signals;
#define EEPROM_SIZE lastAdr + 1

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
LEDSignal signal00;
LEDSignal signal01;
LEDSignal signal02;
LEDSignal signal03;

LEDSignal Signals[num_signals] = {signal00, signal01, signal02, signal03};
uint8_t signalDelay;

// an diese PINs werden die Magnetartikel angeschlossen
uint8_t acc_pin_out_green[num_signals] = {GPIO_NUM_4, GPIO_NUM_17, GPIO_NUM_18, GPIO_NUM_21}; //PIN-Zuordnung
uint8_t acc_pin_out_red[num_signals] = {GPIO_NUM_16, GPIO_NUM_5, GPIO_NUM_19, GPIO_NUM_22};   //PIN-Zuordnung

// Protokollkonstante
#define PROT MM_ACC

// Forward declaration
void switchAcc(uint16_t acc_num);
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
  Serial.println("\r\n\r\nL E D - S i g n a l");
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

    // setzt die Boardnum anfangs auf 1
    params.decoderadr = 1;
    EEPROM.write(adr_decoderadr, params.decoderadr);
    EEPROM.commit();
    EEPROM.write(adr_SrvDel, stdsignaldelay);
    EEPROM.commit();
    // Status der Magnetartikel zu Beginn auf rechts setzen
    for (uint8_t signal = 0; signal < num_signals; signal++)
    {
      EEPROM.write(acc_state + signal, red);
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
  }
  // ab hier werden die Anweisungen bei jedem Start durchlaufen
  // Flags
  got1CANmsg = false;
  SYS_CMD_Request = false;
  statusPING = false;
  initialData2send = false;
  signalDelay = readValfromEEPROM(adr_SrvDel, stdsignaldelay, minsignaldelay, maxsignaldelay);
  for (uint8_t signal = 0; signal < num_signals; signal++)
  {
    // Status der Magnetartikel versenden an die Servos
    Signals[signal].Attach(acc_pin_out_green[signal], acc_pin_out_red[signal], signal);
    // Status der Magnetartikel einlesen in lokale arrays
    Signals[signal].SetLightCurr((colorLED)EEPROM.read(acc_state + signal));
    // Signals mit den PINs verbinden, initialisieren & Artikel setzen wie gespeichert
    Signals[signal].SetDelay(signalDelay);
    Signals[signal].SetcolorLED();
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
  for (uint8_t signal = 0; signal < num_signals; signal++)
  {
    // Status der Magnetartikel einlesen in lokale arrays
    acc_report(signal);
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
  // berechnet die _to_address aus der Adresse und der Protokollkonstante
  uint16_t baseaddress = (params.decoderadr - 1) * num_signals;
  for (uint8_t signal = 0; signal < num_signals; signal++)
  {
    uint16_t to_address = PROT + baseaddress + signal;
    // _to_addresss einlesen in lokales array
    Signals[signal].Set_to_address(to_address);
    acc_report(signal);
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
  // Kanalnummer #1 - Signalverzögerung
  case 1:
  {
    oldval = signalDelay;
    signalDelay = (opFrame[11] << 8) + opFrame[12];
    if (testMinMax(oldval, signalDelay, minsignaldelay, maxsignaldelay))
    {
      EEPROM.write(adr_SrvDel, signalDelay);
      EEPROM.commit();
      for (int signal = 0; signal < num_signals; signal++)
      {
        // neue Verzögerung
        Signals[signal].SetDelay(signalDelay);
      }
    }
    else
    {
      signalDelay = oldval;
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
  opFrame[11] = DEVTYPE_LEDSIGNAL >> 8;
  opFrame[12] = DEVTYPE_LEDSIGNAL;
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
  opFrame[7] = (uint8_t)(Signals[num].Get_to_address() >> 8);
  opFrame[8] = (uint8_t)Signals[num].Get_to_address();
  // Meldung der Lage
  opFrame[9] = Signals[num].GetLightCurr();
  sendCanFrame();
  delay(wait_time); // Delay added just so we can have time to open up
}

// Diese Routine leitet den Positionswechsel einer Weiche/Signal ein.
void switchAcc(uint16_t acc_num)
{
  colorLED set_pos = Signals[acc_num].GetLightDest();
  switch (set_pos)
  {
  case green:
  {
    Signals[acc_num].GoGreen();
  }
  break;
  case red:
  {
    Signals[acc_num].GoRed();
  }
  break;
  }
  acc_report(acc_num);
  Signals[acc_num].SetLightCurr(set_pos);
  EEPROM.write(acc_state + acc_num, (uint8_t)set_pos);
  EEPROM.commit();
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
      /*4*/ 'L', 'E', 'D', '-', 'S', 'i', 'g', 'n',
      /*5*/ 'a', 'l', 0, 0, 0, 0, 0, 0};
  const uint8_t NumLinesKanal01 = 4 * Kanalwidth;
  uint8_t arrKanal01[NumLinesKanal01] = {
      /*1*/ Kanal01, 2, 0, minsignaldelay, 0, maxsignaldelay, 0, signalDelay,
      /*2*/ 'D', 'i', 'm', 's', 'p', 'e', 'e', 'd',
      /*3*/ 0, minsignaldelay + '0', 0, (maxsignaldelay / 100) + '0', (maxsignaldelay - (uint8_t)(maxsignaldelay / 100) * 100) / 10 + '0', (maxsignaldelay - (uint8_t)(maxsignaldelay / 10) * 10) + '0', 0, 'm',
      /*4*/ 's', 0, 0, 0, 0, 0, 0, 0};
  const uint8_t NumLinesKanal02 = 4 * Kanalwidth;
  uint8_t arrKanal02[NumLinesKanal02] = {
      // #2 - WORD immer Big Endian, wie Uhrzeit
      /*1*/ Kanal02, 2, 0, 1, 0, maxadr, 0, params.decoderadr,
      /*2*/ 'M', 'o', 'd', 'u', 'l', 'a', 'd', 'r',
      /*3*/ 'e', 's', 's', 'e', 0, '1', 0, (maxadr / 100) + '0',
      /*4*/ (maxadr - (uint8_t)(maxadr / 100) * 100) / 10 + '0', (maxadr - (uint8_t)(maxadr / 10) * 10) + '0', 0, 'A', 'd', 'r', 0, 0};
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
  for (uint8_t signal = 0; signal < num_signals; signal++)
  {
    // die Servos werden permant abgefragt, ob es ein Delta zwischen
    // tatsächlicher und gewünschter Servostellung gibt
    Signals[signal].Update();
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
