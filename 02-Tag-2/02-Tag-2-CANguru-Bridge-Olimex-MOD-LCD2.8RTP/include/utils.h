
/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <CANguru-Buch@web.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gustav Wostrack
 * ----------------------------------------------------------------------------
 */

#ifndef UTILS_H
#define UTILS_H

#include "display2use.h"
#ifdef LCD28
#include <MOD-LCD.h>
#endif
#include <espnow.h>

#define CAN_EFF_MASK 0x1FFFFFFFU /* extended frame format (EFF) */

enum statustype
{
  even = 0,
  odd,
  undef
};

statustype lastStatus;
statustype currStatus;

bool drawCircle;

// timer
const uint8_t wait_for_ping = 12;
uint8_t secs = 0;
bool gotPINGmsg;
byte onetTimePINGatStart;

Ticker tckr;
#define tckrTime 1.0

//create UDP instance
// EthernetUDP instances to let us send and receive packets over UDP
WiFiUDP UdpOUTWDP;
WiFiUDP UdpOUTGW;

WiFiUDP UdpINWDP;
WiFiUDP UdpINGW;

// Set web server port number to 80
WiFiServer httpServer(80);

// die Portadressen; 15730 und 15731 sind von Märklin festgelegt
// OUT is even
const unsigned int localPortDelta = 2;                                // local port to listen on
const unsigned int localPortoutWDP = 15730;                           // local port to send on
const unsigned int localPortinWDP = 15731;                            // local port to listen on
const unsigned int localPortoutGW = localPortoutWDP + localPortDelta; // local port to send on
const unsigned int localPortinGW = localPortinWDP + localPortDelta;   // local port to listen on

// behandelt die diversen Stadien des ETHERNET-Aufbaus
void WiFiEvent(WiFiEvent_t event)
{
#ifdef OLED
  Adafruit_SSD1306 *displ = getDisplay();
#endif
  switch (event)
  {
  case SYSTEM_EVENT_ETH_START:
#ifdef OLED
    displ->println(F("ETH Started"));
#endif
#ifdef LCD28
    displayLCD("ETHERNET Started");
#endif
    break;
  case SYSTEM_EVENT_ETH_CONNECTED:
#ifdef OLED
    displ->println(F("ETH Connected"));
#endif
#ifdef LCD28
    displayLCD("ETHERNET Connected");
#endif
    //set eth hostname here
    ETH.setHostname("CANguru-Bridge");
    break;
  case SYSTEM_EVENT_ETH_GOT_IP:
#ifdef OLED
    displ->setTextSize(2); // Draw 2X-scale text
    displ->println(ETH.localIP());
    displ->display();
    displ->setTextSize(1); // Draw 1X-scale text
    displ->println(F(""));
    displ->display();
    displ->setTextSize(2); // Draw 2X-scale text
    displ->println(F("Connect!"));
    displ->display();
    displ->setTextSize(1); // Draw 1X-scale text
#endif
#ifdef LCD28
    displayIP(ETH.localIP());
    displayLCD("");
    displayLCD("Connect!");
#endif
    setEthStatus(true);
    break;
  case SYSTEM_EVENT_ETH_DISCONNECTED:
#ifdef OLED
    displ->println(F("ETH Disconnected"));
#endif
#ifdef LCD28
    displayLCD("ETHERNET Disconnected");
#endif
    setEthStatus(false);
    break;
  case SYSTEM_EVENT_ETH_STOP:
#ifdef OLED
    displ->println(F("ETH Stopped"));
#endif
#ifdef LCD28
    displayLCD("ETHERNET Stopped");
#endif
    setEthStatus(false);
    break;
  default:
    break;
  }
#ifdef OLED
  displ->display();
#endif
}

//events
//*********************************************************************************************************

// der Timer steuert das Scannen der Slaves, das Blinken der LED
// sowie das Absenden des PING
void timer1s()
{
  secs++;
  if (get_time4Scanning() == true)
  {
    if (secs > 1)
    {
      set_time4Scanning(false);
    }
  }
  if (secs > wait_for_ping)
    gotPINGmsg = true;
  // startet den fillTheCircle()-Prozess; Ausgabe pulsiert mit einem Kreis
  setbfillRect();
  if (secs % 2 == 0)
  {
    currStatus = even;
  }
  else
  {
    currStatus = odd;
  }
  if (get_WDPseen())
    if (secs % 12 == 0)
    {
      produceFrame(M_CAN_PING_CS2);
      proc2WDP(M_PATTERN);
    }
}

// Start des Timers
void stillAliveBlinkSetup()
{
  tckr.attach(tckrTime, timer1s); // each sec
  lastStatus = undef;
}

// hiermit wird die Aufforderung zum Blinken an die Decoder verschickt
// sowie einmalig die Aufforderung zur Sendung der Initialdaten an die Decoder
// Initialdaten sind bsplw. die anfängliche Stellung der Weichen, Signale oder
// Gleisbesetztmelder; anschließend gibt es eine Wartezeit, die für die einzelnen
// Decodertypen unterschiedlich lang ist
void stillAliveBlinking()
{
  static uint8_t slv = 0;
  uint8_t M_BLINK[] = {0x00, BlinkAlive, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  uint8_t M_IDATA[] = {0x00, sendInitialData, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  if (get_waiting4Handshake() == false)
  {
    if (currStatus != lastStatus)
    {
      lastStatus = currStatus;
      M_BLINK[0x05] = currStatus;
      sendTheData(slv, M_BLINK, CAN_FRAME_SIZE);
      if (get_initialData2send(slv))
      {
        switch (get_decoder_type(slv))
        {
        case DEVTYPE_SERVO:
        case DEVTYPE_SIGNAL:
        case DEVTYPE_LEDSIGNAL:
          sendTheData(slv, M_IDATA, CAN_FRAME_SIZE);
          delay(4 * wait_time_medium);
          break;
        case DEVTYPE_RM:
          sendTheData(slv, M_IDATA, CAN_FRAME_SIZE);
          delay(24 * wait_time_medium);
          break;
        case DEVTYPE_LIGHT:
        case DEVTYPE_CANFUSE:
        case DEVTYPE_GATE:
          break;
        }
        reset_initialData2send(slv);
      }
      slv++;
      if (slv >= get_slaveCnt())
        slv = 0;
    }
  }
}

#endif
