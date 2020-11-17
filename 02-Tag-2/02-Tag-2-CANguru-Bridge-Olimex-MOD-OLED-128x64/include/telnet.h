
/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <CANguru-Buch@web.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gustav Wostrack
 * ----------------------------------------------------------------------------
 */

#ifndef TELNET_H
#define TELNET_H

#include <WiFi.h>
#include <WiFiClient.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>


// behandelt die diversen Stadien des ETHERNET-Aufbaus
// der body ist in utils.h zu finden
void WiFiEvent(WiFiEvent_t event);
// liefert die Info, ob ETHERNET aufgebaut ist
bool getEthStatus();
// setzt den Status, dass ETHERNET aufgebaut ist
void setEthStatus(bool status);

// Klasse, mit dem der Telnet-Client bedient wird
// mit dem Telnet-Client werden die Ausgaben für den CANguru-Server
// dorthin transportiert
class canguruETHClient : public WiFiClient
{
    // öffentliche Routinen
public:
    // Initialisierung des Telnet-Clients
    void telnetInit();
    // Start des Telnet-Client
    bool startTelnetSrv();
    // gibt die Info, ob der Telnet-Client verbunden ist, zurück
    bool getTelnetHasConnected()
    {
        return telnethasconnected;
    };
    // Überprüfung, ob noch eine Verbindung besteht
    bool lostTelnetSrv();
    // stellt die Ziel-IP-Adresse zur Verfügung
    IPAddress getipBroadcast();
    // setzt die IP-Adresse
    void setipBroadcast(IPAddress Broadcast)
    {
        ipBroadcast = Broadcast;
        ipBroadcastSet = true;
    }
    // gibt die Info zurück, ob die IP-Adresse bereits bekannt/gesetzt ist
    bool getIsipBroadcastSet()
    {
        return ipBroadcastSet;
    }
    // überträgt einen String, mit nl wahr zusätzlich mit neuer Zeile
    void printTelnet(bool nl, String str);
    // überträgt ein array mit char
    void printTelnetArr(uint8_t *buffer);
    // überträgt eine Zahl
    void printTelnetInt(bool nl, int num);
    // überträgt zwei Zahlen
    void printTelnetInts(int num1, int num2);
    // wandelt eine IP-Adresse in einen String um
    String ip2strng(IPAddress adr);
    // nicht öffentliche Routinen und Daten
private:
    // setzt Variablen, wenn der Telnet-Client verbunden ist
    void setTelnetClient(WiFiClient tC);
    // lokale Variablen
    IPAddress ipBroadcast;
    WiFiServer telnetServer;
    WiFiClient tlntClnt;
    bool telnethasconnected;
    bool ipBroadcastSet;
};

#endif