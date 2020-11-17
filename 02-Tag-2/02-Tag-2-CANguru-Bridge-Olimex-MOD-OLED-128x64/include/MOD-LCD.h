
/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <CANguru-Buch@web.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gustav Wostrack
 * ----------------------------------------------------------------------------
 */

#include "display2use.h"

#ifdef LCD28

#ifndef MODLCD_H
#define MODLCD_H

#include "Adafruit_ILI9341.h"

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 290
#define TS_MINY 285
#define TS_MAXX 7520
#define TS_MAXY 7510
#define TS_I2C_ADDRESS 0x4d

// This is pinouts for ESP32-EVB
#define TFT_DC 15
#define TFT_CS 17
#define TFT_MOSI 2
#define TFT_MISO 15
#define TFT_CLK 14
//#define TFT_RST 33

// liefert die Adresse des Display
Adafruit_ILI9341 *getDisplay();
// setzt die gleichnamige Variable auf wahr
void setbfillRect();
// erzeugt den pulsierenden Kreis
void fillTheCircle();

// zeigt ein char array auf dem Display an
void displayLCD(const char *txt);
// zeigt einen String auf dem Display an
void displayStringLCD(String str);
// zeigt eine IP-Adresse auf dem Display an
void displayIP(IPAddress ip);
// initialisiert das Display und zeigt das CANguru an
void initDisplayLCD28();

#endif
#endif