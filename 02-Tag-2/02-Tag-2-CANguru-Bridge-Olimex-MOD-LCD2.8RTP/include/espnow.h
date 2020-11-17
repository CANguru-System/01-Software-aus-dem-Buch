
/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <CANguru-Buch@web.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gustav Wostrack
 * ----------------------------------------------------------------------------
 */

#ifndef ESPNOW_H
#define ESPNOW_H

/*
  - The SLAVE Node needs to be initialized as a WIFI_AP you will need to get its MAC via WiFi.softAPmacAddress()
  - THE MASTER Node needs to know the SLAVE Nodes WIFI_SOFT_AP MAC address (from the above comment)
  - You will need the MASTER nodes WIFI_STA MAC address via WiFi.macAddress()
  - The SLAVE needs the MASTER nodes WIFI_STA MAC address (from above line)
 */

#include "display2use.h"
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <Ticker.h>
#include <Adafruit_SSD1306.h>
#include <telnet.h>

/*
            M A S T E R
*/

const uint8_t WIFI_CHANNEL = 0;
const uint8_t maxSlaves = 20;
const uint8_t macLen = 6;
const uint8_t CAN_FRAME_SIZE = 13; /* maximum datagram size */

// Infos über ESPNow
struct slaveInfoStruct
{
  esp_now_peer_info_t slave;
  esp_now_peer_info_t *peer;
  bool initialData2send;
  bool gotHandshake;
  uint8_t no;
  uint8_t decoderType;
};

// zur Verarbeitung der MAC-Adressen
struct macAddressesstruct
{
  uint8_t peer_addr[macLen];
  uint64_t mac;
};

// Infos über diesen Decoder
struct decoderStruct
{
  bool isType;
  uint8_t decoder_no;
};

// Befehle, die benutzt werden
enum patterns
{
  M_GLEISBOX_MAGIC_START_SEQUENCE,
  M_STOP,
  M_GO,
  M_BIND,
  M_VERIFY,
  M_FUNCTION,
  M_CAN_PING,
  M_CAN_PING_CS2,
  M_READCONFIG,
  M_STARTCONFIG,
  M_FINISHCONFIG,
  M_GETCONFIG1,
  M_GETCONFIG2,
  M_SIGNAL
};

// Kommunikationslinien
enum CMD
{
  toCAN,
  toClnt,
  toWDP,
  fromWDP,
  fromWDP2CAN,
  fromCAN2WDP,
  fromClnt
};

enum outUDP
{
  GW,
  WDP,
  Clnt
};

#ifdef OLED
// gibt einen gefüllten Kreis auf dem Display aus
void fillTheCircle();
// Initialisierung des Display
void initDisplay_OLED();
// Übergibt die Adresse des Display
Adafruit_SSD1306 *getDisplay();
// setzt die gleichnamige Variable auf wahr
void setbfillRect();
#endif
// identifiziert einen Slave anhand seiner UID
uint8_t matchUID(uint8_t *buffer);
// ESPNow wird initialisiert
void espInit();
// schaltet den Prozess Scan4Slaves() ein oder aus
void set_time4Scanning(bool t4s);
// gibt an, ob der Prozess Scan4Slaves() läuft oder nicht
bool get_time4Scanning();
// schaltet den Prozess waiting4Handshake ein oder aus; nach dem Erfassen der Slaves
// gibt es einen abschließenden Handshake mit jedem Slave
void set_waiting4Handshake(bool w4s);
// gibt an, ob der Prozess waiting4Handshake läuft oder nicht
bool get_waiting4Handshake();
// Kommunikationsroutine zu WDP (Win-DigiPet)
void proc2WDP(uint8_t *buffer);
// Kommunikationsroutine zu den CAN-Komponenten (momentan nur die Gleisbox)
void proc2CAN(uint8_t *buffer, CMD dir);
// Laden der lokomotive.cs2-Datei
void reveiveLocFile();
// Übertragung von Frames zum CANguru-Server zur dortigen Ausgabe
void printUDPCAN(uint8_t *buffer, CMD dir);
// Ausgaberoutine
void sendOutUDP(outUDP udp, uint8_t *buffer);
// Anzeige, dass WDP gestartet werden kabb
void goWDP();
// gibt die Anzahl der gefundenen Slaves zurück
uint8_t get_slaveCnt();
// gibt an, ob WDP gestartet ist
bool get_WDPseen();
// setzt, dass WDP gestartet ist
void set_WDPseen(bool WDP);
// setzt die Variable cntConfig auf Null
void set_cntConfig();
// fordert einen Slave dazu auf, Anfangsdaten bekannt zu geben
void set_initialData2send(uint8_t slave);
// quittiert, dass Anfangsdaten übertragen wurden
void reset_initialData2send(uint8_t slave);
// gibt zurück, ob noch Anfangsdaten von einem Slave zu liefern sind
bool get_initialData2send(uint8_t slave);
// gibt zurück, um welchen Decodertypen es sich handelt
uint8_t get_decoder_type(uint8_t no);
// quittiert, dass eine eingegangene Meldung gelesen wurde
void resetgot1CANmsg(uint8_t slave);
// liefert die Struktur slaveInfo zurück
slaveInfoStruct get_slaveInfo(uint8_t slave);
// gibt eine MAC-Adresse aus
void printMac(uint8_t m[macLen]);
// vergleicht zwei MAC-Adressen
bool macIsEqual(uint8_t m0[macLen], uint8_t m1[macLen]);
// Scannt nach Slaves
void Scan4Slaves();
// setzt die vorgegebene MAC-Adresse des Masters
void initVariant();
// addiert und registriert gefundene Slaves
void addSlaves();
// steuert den Registrierungsprozess der Slaves
void espNowProc();
// sendet daten über ESPNow
void sendTheData(const uint8_t slave, const uint8_t *data, size_t len);
// nach dem Versand von Meldungen können hier Fehlermeldungen abgerufen werden
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
// empfängt Daten über ESPNow
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len);

#endif
