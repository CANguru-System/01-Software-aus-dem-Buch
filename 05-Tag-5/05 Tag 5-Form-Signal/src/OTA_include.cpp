#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include "EEPROM.h"
#include <ArduinoOTA.h>
#include <OTA_include.h>
#include "ticker.h"

Ticker tckrOTA;
const float tckrOTATime = 0.01;

enum blinkStatus
{
  blinkFast = 0, // wartet auf Password
  blinkSlow,     // wartet auf WiFi
  blinkNo        // mit WiFi verbunden
};
blinkStatus blink;

// der Timer steuert das Scannen der Slaves, das Blinken der LED
// sowie das Absenden des PING
void timerOTA()
{
  static uint8_t secs = 0;
  static uint8_t slices = 0;
  slices++;
  switch (blink)
  {
  case blinkFast:
    if (slices >= 10)
    {
      slices = 0;
      secs++;
    }
    break;
  case blinkSlow:
    if (slices >= 40)
    {
      slices = 0;
      secs++;
    }
    break;
  case blinkNo:
      secs = 2;
    break;
  }
  if (secs % 2 == 0)
    // turn the LED on by making the voltage HIGH
    digitalWrite(BUILTIN_LED, HIGH);
  else
    // turn the LED off by making the voltage LOW
    digitalWrite(BUILTIN_LED, LOW);
}

void Connect2WiFiandOTA()
{
  tckrOTA.attach(tckrOTATime, timerOTA); // each sec
  blink = blinkFast;
  String ssid = "";
  String password = "";
  ssid = EEPROM.readString(adr_ssid);
  password = EEPROM.readString(adr_password);
  char ssidCh[ssid.length() + 1];
  ssid.toCharArray(ssidCh, ssid.length() + 1);
  char passwordCh[password.length() + 1];
  password.toCharArray(passwordCh, password.length() + 1);
  // Connect to Wi-Fi network with SSID and password
  Serial.print("Verbinde mit dem Netzwerk -");
  Serial.print(ssidCh);
  Serial.println("-");
  Serial.print("Mit dem Passwort -");
  Serial.print(passwordCh);
  Serial.println("-");
  WiFi.begin(ssidCh, passwordCh);
  uint8_t round = 0;
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Keine Verbindung! Neuer Versuch ...");
    delay(5000);
    round++;
    if (round > 4)
    {
      Serial.println("Keine Verbindung! Neustart ...");
      ESP.restart();
    }
  }

  ArduinoOTA
      .onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
          type = "sketch";
        else // U_SPIFFS
          type = "filesystem";

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        Serial.println("Start updating " + type);
      })
      .onEnd([]() {
        Serial.println("\nEnd");
      })
      .onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
      })
      .onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
          Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)
          Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR)
          Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR)
          Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR)
          Serial.println("End Failed");
      });

  ArduinoOTA.begin();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  blink = blinkSlow;
  while (true)
  {
    ArduinoOTA.handle();
  }
}
