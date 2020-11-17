
/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <CANguru-Buch@web.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gustav Wostrack
 * ----------------------------------------------------------------------------
 */
#include <Arduino.h>
#include <ETH.h>
#include "CANguruDefs.h"
#include "display2use.h"
#include <SPI.h>
#include <Wire.h>
#include "ESP32SJA1000.h"
#include <espnow.h>
#include <telnet.h>
#include <Adafruit_GFX.h>
#ifdef OLED
#include <Adafruit_SSD1306.h>
#endif
#ifdef LCD28
#include "MOD-LCD.h"
#endif

// buffer for receiving and sending data
uint8_t M_PATTERN[] = {0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t lastmfxUID[] = {0x00, 0x00, 0x00, 0x00};

#define httpBufferLength 0x03FF
char httpBuffer[httpBufferLength + 10];
char *locofile;

// IP address to send UDP data to.
// it can be ip address of the server or
// a network broadcast address
// here is broadcast address
IPAddress ipOWN(255, 255, 255, 255);
canguruETHClient telnetClient;

bool locofileread;
bool initialDataAlreadySent;

byte locid;
byte cvIndex;

// überträgt die Daten eines CAN-Frames an den CANguru-Server, der diese
// Daten dann aufbereitet und darstellt
// da die Daten auf dem CANguru-Server als String empfangen werden, muss
// gewisse Vorsorge für nicht darstellbare Zeichen getroffen werden. Sie wird
// aus jedem Einzelzeichen ein Doppelzeichen; so wird jedem Zeichen ein Fluchtsymbol
// vorangestellt, dass aussagt, inwiefern das eigentliche Zeichen manipuliert
// wurde. Ziel ist, das jedes Zeichen wie ein darstellbares Zeichen aussieht.
// Die Bedeutung der Fluchtsymbole
// &: das erste übertragene Zeichen; es folgt die Info, für wen der Frame bestimmt ist
// %: das Zeichen ist kleiner als ' '; eine '0' wird addiert;
// &: das Zeichen ist größer als alle darstellbaren Zeichen; eine hex 80 wird subtrahiert
// $: das Zeichen ist darstellabr, keine weitere Manipulation.
// Auf dem CANguru-Server werden alle Manipulationen wieder rückgeführt, so dass
// die Zeichen wieder im Originalzustand hergestellt sind
//
void printCANFrame(uint8_t *buffer, CMD dir)
{
  char dirCh = dir + '0';
  uint8_t tmpBuffer[] = {'&', dirCh,
                         /*00*/ 0x00, 0x00,
                         /*01*/ 0x00, 0x00,
                         /*02*/ 0x00, 0x00,
                         /*03*/ 0x00, 0x00,
                         /*04*/ 0x00, 0x00,
                         /*05*/ 0x00, 0x00,
                         /*06*/ 0x00, 0x00,
                         /*07*/ 0x00, 0x00,
                         /*08*/ 0x00, 0x00,
                         /*09*/ 0x00, 0x00,
                         /*10*/ 0x00, 0x00,
                         /*11*/ 0x00, 0x00,
                         /*12*/ 0x00, 0x00,
                         /*13*/ '\r', '\n'};
  uint8_t index;
  for (uint8_t ch = 0; ch < CAN_FRAME_SIZE; ch++)
  {
    index = 2 * ch + 2;
    if (buffer[ch] < 0x20)
    {
      tmpBuffer[index] = '%';
      tmpBuffer[index + 1] = buffer[ch] + '0';
    }
    else
    {
      if (buffer[ch] >= 0x80)
      {
        tmpBuffer[index] = '&';
        tmpBuffer[index + 1] = buffer[ch] - 0x80;
      }
      else
      {
        tmpBuffer[index] = '$';
        tmpBuffer[index + 1] = buffer[ch];
      }
    }
  }
  telnetClient.printTelnetArr(tmpBuffer);
  delay(10);
}

// ein CAN-Frame wird erzeugt, der Parameter noFrame gibt an, welche Daten
// in den Frame zu kopieren sind
void produceFrame(patterns noFrame)
{
  memset(M_PATTERN, 0x0, CAN_FRAME_SIZE);
  M_PATTERN[2] = 0x03;
  switch (noFrame)
  {
  case M_GLEISBOX_MAGIC_START_SEQUENCE:
    M_PATTERN[1] = 0x36;
    M_PATTERN[4] = 0x05;
    M_PATTERN[9] = 0x11;
    break;
  case M_GO:
    M_PATTERN[1] = 0x00;
    M_PATTERN[4] = 0x05;
    M_PATTERN[9] = 0x01;
    break;
  case M_STOP:
    M_PATTERN[1] = 0x00;
    M_PATTERN[4] = 0x05;
    M_PATTERN[9] = 0x00;
    break;
  case M_BIND:
    M_PATTERN[1] = 0x04;
    M_PATTERN[4] = 0x06;
    M_PATTERN[9] = 0x00;
    break;
  case M_VERIFY:
    M_PATTERN[1] = 0x06;
    M_PATTERN[4] = 0x06;
    M_PATTERN[9] = 0x00;
    break;
  case M_FUNCTION:
    M_PATTERN[1] = 0x0C;
    M_PATTERN[4] = 0x06;
    M_PATTERN[7] = 0x04;
    break;
  case M_CAN_PING:
    M_PATTERN[1] = 0x30;
    M_PATTERN[2] = 0x47;
    M_PATTERN[3] = 0x11;
    break;
  case M_CAN_PING_CS2:
    M_PATTERN[1] = 0x31;
    M_PATTERN[2] = 0x47;
    M_PATTERN[3] = 0x11;
    M_PATTERN[4] = 0x08;
    M_PATTERN[9] = 0x03;
    M_PATTERN[10] = 0x08;
    M_PATTERN[11] = 0xFF;
    M_PATTERN[12] = 0xFF;
    break;
  case M_READCONFIG:
    M_PATTERN[1] = 0x0E;
    M_PATTERN[4] = 0x07;
    M_PATTERN[7] = 0x40;
    break;
  case M_STARTCONFIG:
    M_PATTERN[1] = 0x51;
    M_PATTERN[4] = 0x02;
    M_PATTERN[5] = 0x01;
    break;
  case M_FINISHCONFIG:
    M_PATTERN[1] = 0x51;
    M_PATTERN[4] = 0x01;
    M_PATTERN[5] = 0x00;
    break;
  case M_GETCONFIG1:
    M_PATTERN[1] = 0x40;
    M_PATTERN[4] = 0x08;
    M_PATTERN[5] = 0x6C;
    M_PATTERN[6] = 0x6F;
    M_PATTERN[7] = 0x6B;
    M_PATTERN[8] = 0x73;
    break;
  case M_GETCONFIG2:
    M_PATTERN[1] = 0x41;
    M_PATTERN[4] = 0x08;
    M_PATTERN[5] = 0x6C;
    M_PATTERN[6] = 0x6F;
    M_PATTERN[7] = 0x6B;
    M_PATTERN[8] = 0x73;
    break;
  case M_SIGNAL:
    M_PATTERN[1] = 0x50;
    M_PATTERN[4] = 0x01;
    break;
  }
}

#include <utils.h>

// sendet den Frame, auf den der Zeiger buffer zeigt, über ESPNow
// an alle Slaves
void send2AllClients(uint8_t *buffer)
{
  uint8_t slaveCnt = get_slaveCnt();
  for (uint8_t s = 0; s < slaveCnt; s++)
  {
    sendTheData(s, buffer, CAN_FRAME_SIZE);
  }
}

// wenn aus der UID des Frames ein Slave identifizierbar ist, wird der
// Frame nur an diesen Slave geschickt, ansinsten an alle
void send2OneClient(uint8_t *buffer)
{
  uint8_t ss = matchUID(buffer);
  if (ss == 0xFF)
    send2AllClients(buffer);
  else
    sendTheData(ss, buffer, CAN_FRAME_SIZE);
}

// sendet einen CAN-Frame an den Teilnehmer udp und gibt ihn anschließend aus
void sendOutUDP(outUDP udp, uint8_t *buffer)
{
  switch (udp)
  {
  case GW:
    UdpOUTGW.beginPacket(telnetClient.getipBroadcast(), localPortoutGW);
    UdpOUTGW.write(buffer, CAN_FRAME_SIZE);
    UdpOUTGW.endPacket();
    break;
  case WDP:
    UdpOUTWDP.beginPacket(telnetClient.getipBroadcast(), localPortoutWDP);
    UdpOUTWDP.write(buffer, CAN_FRAME_SIZE);
    UdpOUTWDP.endPacket();
    printCANFrame(buffer, toWDP);
    break;
  case Clnt:
    switch (buffer[0x01])
    {
    case CONFIG_Status:
      send2OneClient(buffer);
      break;
    case SYS_CMD:
      switch (buffer[0x09]){
        case SYS_STAT:
          send2OneClient(buffer);
          break;
        default:
          proc2CAN(buffer, toCAN);
          send2AllClients(buffer);
          break;
      }
      break;
    // send to all
    default:
      send2AllClients(buffer);
      break;
    }
    printCANFrame(buffer, toClnt);
    break;
  }
}

// Standardroutine; diverse Anfangsbedingungen werden hergestellt
void setup()
{
  Serial.begin(bdrMonitor);
//  Serial.setDebugOutput(true);
  Serial.println("\r\n\r\nC A N g u r u - B r i d g e");
  drawCircle = false;
#ifdef OLED
  // das Display wird initalisiert
  initDisplay_OLED();
  // das Display zeigt den Beginn der ersten Routinen
  Adafruit_SSD1306 *displ = getDisplay();
  displ->display();
#endif
#ifdef LCD28
  // das Display wird initalisiert
  initDisplayLCD28();
#endif
  // noch nicht nach Slaves scannen
  set_time4Scanning(false);
  // der Timer wird initialisiert
  stillAliveBlinkSetup();
  // start the CAN bus at 250 kbps
  if (!CAN.begin(250E3))
  {
#ifdef OLED
    displ->println(F("Starting CAN failed!"));
    displ->display();
    while (1)
      delay(10);
  }
  else
  {
    displ->println(F("Starting CAN was successful!"));
    displ->display();
#endif
#ifdef LCD28
    displayLCD("Starting CAN failed!");
    while (1)
      delay(10);
  }
  else
  {
    displayLCD("CAN is running!");
#endif
  }
  // ESPNow wird initialisiert
  espInit();
  // das gleiche mit ETHERNET
  WiFi.onEvent(WiFiEvent);
  ETH.begin();
  // Variablen werden gesetzt
  gotPINGmsg = false;
  onetTimePINGatStart = 1;
  set_WDPseen(false);
  set_cntConfig();
  locofileread = false;
  initialDataAlreadySent = false;
  locid = 5;
  //start the telnetClient
  telnetClient.telnetInit();
  //This initializes udp and transfer buffer
  if (UdpINWDP.begin(localPortinWDP) == 0)
  {
#ifdef OLED
    displ->println(F("ERROR inWDP"));
    displ->display();
#endif
#ifdef LCD28
#endif
  }
  if (UdpINGW.begin(localPortinGW) == 0)
  {
#ifdef OLED
    displ->println(F("ERROR inGW"));
    displ->display();
#endif
#ifdef LCD28
    displayLCD("ERROR inWDP");
#endif
  }
  if (UdpOUTGW.begin(localPortoutGW) == 0)
  {
#ifdef OLED
    displ->println(F("ERROR outWDP"));
    displ->display();
#endif
#ifdef LCD28
    displayLCD("ERROR inGW");
#endif
  }
  if (UdpOUTWDP.begin(localPortoutWDP) == 0)
  {
#ifdef OLED
    displ->println(F("ERROR outGW"));
    displ->display();
#endif
#ifdef LCD28
    displayLCD("ERROR outGW");
#endif
  }
  // start the server
  httpServer.begin();
  //
  // app starts here
  //
}

// damit wird die Gleisbox zum Leben erweckt
void send_start_60113_frames()
{
  produceFrame(M_GLEISBOX_MAGIC_START_SEQUENCE);
  proc2CAN(M_PATTERN, toCAN);
  // GO or STOP
  delay(250);
  produceFrame(M_GO);
  proc2CAN(M_PATTERN, toCAN);
  delay(250);
}

// prüft, ob es Slaves gibt und sendet den CAN-Frame buffer dann an die Slaves
void proc2Clnts(uint8_t *buffer)
{
  // to Client
  if (get_slaveCnt() > 0)
    sendOutUDP(Clnt, buffer);
}

// sendet den CAN-Frame buffer an WDP
void proc2WDP(uint8_t *buffer)
{
  // to WDP
  sendOutUDP(WDP, buffer);
}

// sendet den CAN-Frame buffer über den CAN-Bus an die Gleisbox
void proc2CAN(uint8_t *buffer, CMD dir)
{
  // CAN uses (network) big endian format
  // Maerklin TCP/UDP Format: always 13 (CAN_FRAME_SIZE) bytes
  //   byte 0 - 3  CAN ID
  //   byte 4      DLC
  //   byte 5 - 12 CAN data
  //
  printCANFrame(buffer, dir);
  uint32_t canid;
  memcpy(&canid, buffer, 4);
  // CAN uses (network) big endian format
  canid = ntohl(canid);
  int len = buffer[4];
  // send extended packet: id is 29 bits, packet can contain up to 8 bytes of data
  CAN.beginExtendedPacket(canid);
  CAN.write(&buffer[5], len);
  CAN.endPacket();
}

// wird benötigt, um mfx-Loks zu erkennen
void sendSignal()
{
  produceFrame(M_SIGNAL);
  sendOutUDP(GW, M_PATTERN);
}

// wird für die Erkennung von mfx-Loks gebraucht
void bindANDverify(uint8_t *buffer)
{
  // BIND
  produceFrame(M_BIND);
  M_PATTERN[10] = locid;
  // MFX-UID
  memcpy(lastmfxUID, &buffer[5], 4);
  memcpy(&M_PATTERN[5], lastmfxUID, 4);
  proc2CAN(M_PATTERN, toCAN);
  delay(10);
  // VERIFY
  produceFrame(M_VERIFY);
  M_PATTERN[10] = locid;
  // MFX-UID
  memcpy(&M_PATTERN[5], lastmfxUID, 4);
  proc2CAN(M_PATTERN, toCAN);
  delay(10);
  // FUNCTION
  produceFrame(M_FUNCTION);
  M_PATTERN[8] = locid;
  M_PATTERN[10] = 0x01;
  proc2CAN(M_PATTERN, toCAN);
  produceFrame(M_FUNCTION);
  M_PATTERN[8] = locid;
  M_PATTERN[10] = 0x00;
  delay(10);
  proc2CAN(M_PATTERN, toCAN);
}

// Anfordrung an den CANguru-Server zur Übertragung der lokomotive.cs2-Datei
void askforConfigData(byte lineNo)
{
  //
  produceFrame(M_GETCONFIG2);
  M_PATTERN[9] = lineNo;
  M_PATTERN[10] = httpBufferLength & 0x00FF;
  M_PATTERN[11] = (httpBufferLength & 0xFF00) >> 8;
  //
  // to Gateway
  sendOutUDP(GW, M_PATTERN);
}

// Auswertung der config-Datei
char readConfig(char index)
{
  produceFrame(M_READCONFIG);
  M_PATTERN[8] = locid;
  M_PATTERN[9] = index;
  M_PATTERN[9] += 0x04;
  M_PATTERN[10] = 0x03;
  M_PATTERN[11] = 0x01;
  proc2CAN(&M_PATTERN[0], toCAN);
  delay(10);
  return M_PATTERN[9];
}

// Anfrage, wieviele Daten sind zu übertragen
uint32_t getDataSize()
{
  uint8_t UDPbuffer[] = {0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  //
  produceFrame(M_GETCONFIG1);
  // to Gateway
  sendOutUDP(GW, M_PATTERN);
  uint16_t packetSize = 0;
  uint8_t loctimer = secs;
  // maximal 2 Sekunden warten
  while (packetSize == 0 && secs < loctimer + 2)
  {
    packetSize = UdpINGW.parsePacket();
  }
  if (packetSize)
  {
    // read the packet into packetBufffer
    UdpINGW.read(UDPbuffer, CAN_FRAME_SIZE);
    // and respond via UDP
    uint32_t cF = 0;
    memcpy(&cF, &UDPbuffer[5], 4);
    return ntohl(cF);
  }
  return 0;
}

// Übertragung der lokomotive.cs2-Datei vom CANguru-Server
// die CANguru-Bridge hält diese Daten und leitet sie bei Anfrage an
// den WDP weiter
void reveiveLocFile()
{
  // ruft Daten ab
  uint32_t cntFrame = getDataSize();
  if (cntFrame == 0)
  {
    locofileread = false;
    return;
  }
  locofile = new char[cntFrame];
  // Configdaten abrufen
  uint16_t packetSize = 0;
  uint16_t inBufferCnt = 0;
  byte lineNo = 0;
  //
  locofileread = true;
  while (inBufferCnt < cntFrame)
  {
    askforConfigData(lineNo);
    packetSize = 0;
    while (packetSize == 0)
    {
      packetSize = UdpINGW.parsePacket();
    }
    if (packetSize)
    {
      // read the packet into packetBuffer
      // from gateway via udp
      UdpINGW.read(httpBuffer, packetSize);
      httpBuffer[packetSize] = 0x00;
      lineNo++;
      //  printTelnet(false, ".");
      // write the packet to local file
      memcpy(&locofile[inBufferCnt], &httpBuffer[0], packetSize);
      inBufferCnt += packetSize;
    }
  }
}

// die Routin antwortet auf die Anfrage des CANguru-Servers mit CMD 0x88;
// damit erhält er die IP-Adresse der CANguru-Bridge und kann dann
// damit eine Verbindung aufbauen
void proc_IP2GW()
{
  uint8_t UDPbuffer[CAN_FRAME_SIZE]; //buffer to hold incoming packet,
  if (telnetClient.getIsipBroadcastSet())
    return;
  int packetSize = UdpINGW.parsePacket();
  // if there's data available, read a packet
  if (packetSize)
  {
    // read the packet into packetBuffer
    UdpINGW.read(UDPbuffer, CAN_FRAME_SIZE);
    // send received data via ETHERNET
    switch (UDPbuffer[0x1])
    {
    case 0x88:
      telnetClient.setipBroadcast(UdpINGW.remoteIP());
      UDPbuffer[0x1]++;
      sendOutUDP(GW, UDPbuffer);
      break;
    }
  }
}

// Behandlung der Kommandos, die der CANguru-Server aussendet
void proc_fromGW2CANandClnt()
{
  uint8_t UDPbuffer[CAN_FRAME_SIZE]; //buffer to hold incoming packet,
  int packetSize = UdpINGW.parsePacket();
  // if there's data available, read a packet
  if (packetSize)
  {
    // read the packet into packetBufffer
    UdpINGW.read(UDPbuffer, CAN_FRAME_SIZE);
    // send received data via usb and CAN
    switch (UDPbuffer[0x1])
    {
    case SYS_CMD:
    case 0x02:
    case 0x04:
    case 0x06:
    case 0x36:
    case 0x3A:
      proc2Clnts(UDPbuffer);
      break;
    case 0x50:
      // received next locid
      locid = UDPbuffer[0x05];
      sendSignal();
      break;
    case 0x51:
      // there is a new file lokomotive.cs2 ro send
      sendSignal();
      reveiveLocFile();
      break;
    // PING
    case 0x30:
      gotPINGmsg = true;
      break;
    case 0x62:
      ESP.restart();
      break;
    }
  }
}

// sendet CAN-Frames vom WDP zum CAN (Gleisbox)
void proc_fromWDP2CAN()
{
  uint8_t UDPbuffer[CAN_FRAME_SIZE]; //buffer to hold incoming packet,
  int packetSize = UdpINWDP.parsePacket();
  // if there's data available, read a packet
  if (packetSize)
  {
    set_WDPseen(true);
    // read the packet into packetBufffer
    UdpINWDP.read(UDPbuffer, CAN_FRAME_SIZE);
    // send received data via usb and CAN
    proc2CAN(UDPbuffer, fromWDP2CAN);
    // and respond via UDP
    switch (UDPbuffer[0x01])
    {
    case PING:
      produceFrame(M_CAN_PING_CS2);
      proc2WDP(M_PATTERN);
      break;
    case SWITCH_ACC:
      // send received data via wifi to clients
      proc2Clnts(UDPbuffer);
      break;
    case S88_Polling:
      UDPbuffer[0x01]++;
      UDPbuffer[0x04] = 7;
      proc2WDP(UDPbuffer);
      break;
    case S88_EVENT:
      UDPbuffer[0x01]++;
      UDPbuffer[0x09] = 0x00; // is free
      UDPbuffer[0x04] = 8;
      proc2WDP(UDPbuffer);
      break;
    }
  }
}

// sendet CAN-Frames vom  CAN (Gleisbox) zum WDP
void proc_fromCAN2WDPandGW()
{
  int packetSize = CAN.parsePacket();
  if (packetSize)
  {
    // read a packet from CAN
    uint8_t i = 0;
    uint8_t UDPbuffer[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint32_t canid = CAN.packetId();
    canid &= CAN_EFF_MASK;
    canid = htonl(canid);
    memcpy(UDPbuffer, &canid, 4);
    UDPbuffer[4] = packetSize;
    while (CAN.available())
    {
      UDPbuffer[5 + i] = CAN.read();
      i++;
    }
    // send UDP
    switch (UDPbuffer[1])
    {
    case 0x30: // PING
      sendOutUDP(GW, UDPbuffer);
      break;
    case 0x31: // PING
      sendOutUDP(GW, UDPbuffer);
      if (UDPbuffer[12] == DEVTYPE_GB && get_slaveCnt() == 0 && get_waiting4Handshake() == true)
      {
#ifdef OLED
        Adafruit_SSD1306 *displ = getDisplay();
        displ->println(F(" -- No Slaves!"));
        displ->display();
#endif
#ifdef LCD28
      displayLCD(" -- No Slaves!");
#endif
        goWDP();
        set_waiting4Handshake(false);
      }
      break;
    case 0x03:
      // mfxdiscovery war erfolgreich
      if (UDPbuffer[4] == 0x05)
      {
        bindANDverify(UDPbuffer);
        // an gateway den anfang melden
      }
      break;
    case 0x07:
      produceFrame(M_STARTCONFIG);
      // LocID
      M_PATTERN[6] = UDPbuffer[10];
      // MFX-UID
      memcpy(&M_PATTERN[7], lastmfxUID, 4);
      // to Gateway
      sendOutUDP(GW, M_PATTERN);
      cvIndex = readConfig(0);
      break;
    case 0x0F:
      // Rückmeldungen von config
      if (UDPbuffer[10] == 0x03)
      {
        if (UDPbuffer[11] == 0x00)
        {
          // das war das letzte Zeichen
          // an gateway den schluss melden
          produceFrame(M_FINISHCONFIG);
          // to Gateway
          sendOutUDP(GW, M_PATTERN);
        }
        else
        {
          // to Gateway
          sendOutUDP(GW, UDPbuffer);
          cvIndex = readConfig(cvIndex);
        }
      }
      break;
    default:
      if (get_WDPseen())
        proc2WDP(UDPbuffer);
      break;
    }
  }
}

// sendet lokomotive.cs2 auf Anforderung des CANguru-Servers
void sendConfigData(WiFiClient client)
{
  // ruft Daten ab
  uint32_t cntFrame = getDataSize();
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/octet-stream");
  client.print("Content-Length: ");
  client.println(cntFrame);
  client.println();
  // Configdaten abrufen
  String line;
  uint16_t packetSize = 0;
  uint16_t inBufferCnt = 0;
  //
  while (inBufferCnt < cntFrame)
  {
    packetSize = httpBufferLength;
    if (packetSize)
    {
      // read the packet into packetBuffer
      // from gateway via udp
      memcpy(&httpBuffer[0], &locofile[inBufferCnt], packetSize);
      httpBuffer[packetSize] = 0x00;
      // write the packet to WDP via http
      client.print(httpBuffer);
      client.flush();
      inBufferCnt += packetSize;
    }
  }
}

// WDP kommuniziert auch über HTTP (für die Datei lokomotve.cs2)
void proc_fromWebServer()
{
  // listen for incoming clients
  WiFiClient httpclient = httpServer.available();
  if (httpclient)
  {
    telnetClient.printTelnet(true, "New HTTP client");
    // an http request ends with a blank line
    bool currentLineIsBlank = true;
    while (httpclient.connected())
    {
      if (httpclient.available())
      {
        char c = httpclient.read();
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank)
        {
          // send the required response
          sendConfigData(httpclient);
          break;
        }
        if (c == '\n')
        {
          // you're starting a new line
          currentLineIsBlank = true;
        }
        else if (c != '\r')
        {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(5);
    // close the connection:
    httpclient.stop();
    telnetClient.printTelnet(true, "HTTP client disconnected");
    if (initialDataAlreadySent == false)
    {
      initialDataAlreadySent = true;
      uint8_t no_slv = get_slaveCnt();
      for (uint8_t slv = 0; slv < no_slv; slv++)
      {
        set_initialData2send(slv);
      }
    }
  }
}

// Antwort auf die PING-Anfrage
void proc_PING()
{
  if (gotPINGmsg)
  {
    gotPINGmsg = false;
    secs = 0;
    if ((onetTimePINGatStart == 0) || (get_WDPseen() && telnetClient))
    {
      produceFrame(M_CAN_PING);
      proc2CAN(M_PATTERN, toCAN);
      proc2Clnts(M_PATTERN);
      if (onetTimePINGatStart > 1)
        proc2WDP(M_PATTERN);
      onetTimePINGatStart++;
    }
  }
}

// Start des Telnet-Servers
void startTelnetserver()
{
  if (telnetClient.getTelnetHasConnected() == false)
  {
    if (telnetClient.startTelnetSrv())
    {
      delay(100);
      reveiveLocFile();
      if (locofileread)
        telnetClient.printTelnet(true, "Read lokomotive.cs2");
      else
        telnetClient.printTelnet(true, "Unable to read lokomotive.cs2");
      telnetClient.printTelnet(true, "");
      send_start_60113_frames();
      // erstes PING soll schnell kommen
      secs = wait_for_ping;
      onetTimePINGatStart = 0;
      set_time4Scanning(true);
    }
  }
}

// Meldung, dass WDP gestartet werden kann
void goWDP()
{
  telnetClient.printTelnet(true, "Start WDP");
  drawCircle = true;
}

// standard-Hauptprogramm
void loop()
{
  // die folgenden Routinen werden ständig aufgerufen
  stillAliveBlinking();
  espNowProc();
  proc_IP2GW();
  if (drawCircle)
    // diese nur, wenn drawRect wahr ist (nicht zu Beginn des Programmes)
    fillTheCircle();
  if (getEthStatus() == true)
  {
    // diese nur nach Aufbau der ETHERNET-Verbindung
    startTelnetserver();
    proc_fromGW2CANandClnt();
    proc_fromCAN2WDPandGW();
    proc_fromWDP2CAN();
    proc_PING();
    proc_fromWebServer();
  }
}
