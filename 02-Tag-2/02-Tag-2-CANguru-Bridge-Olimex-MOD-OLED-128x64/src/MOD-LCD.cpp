
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

#include "MOD-LCD.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include <Adafruit_STMPE610.h>
#include <CANguru.h>

// Variablen
Adafruit_STMPE610 ts = Adafruit_STMPE610();
Adafruit_ILI9341 display9341 = Adafruit_ILI9341(TFT_CS, TFT_DC);
const int16_t x_pos = 15;
const int16_t y_pos = 5;
int16_t height, middleX, middleY;
bool bfillRect;

// liefert die Adresse des Display
Adafruit_ILI9341 *getDisplay()
{
  return &display9341;
}

// setzt die gleichnamige Variable auf wahr
void setbfillRect() {
  bfillRect = true;
}

// erzeugt den pulsierenden Kreis
void fillTheCircle()
{
  static uint16_t color = 0;
  if (bfillRect == false)
    return;
  display9341.fillScreen(ILI9341_BLACK);
  for (int16_t i = middleY; i > 0; i -= 3)
  {
    // The INVERSE color is used so circles alternate white/black
    display9341.drawCircle(middleX, middleY, i, color);
    color++;
  }
  bfillRect = false;
}

// zeigt ein char array auf dem Display an
void displayLCD(const char *txt)
{
  int y = display9341.getCursorY();
  if (y > height)
  {
    display9341.fillScreen(ILI9341_BLACK);
    display9341.setCursor(x_pos, y_pos);
  }
  else
    display9341.setCursor(x_pos, y);
  display9341.println(txt);
}

// zeigt einen String auf dem Display an
void displayStringLCD(String str)
{
  int y = display9341.getCursorY();
  if (y > height)
  {
    display9341.fillScreen(ILI9341_BLACK);
    display9341.setCursor(x_pos, y_pos);
  }
  else
    display9341.setCursor(x_pos, y);
  display9341.println(str);
}

// zeigt eine IP-Adresse auf dem Display an
void displayIP(IPAddress ip)
{
  int y = display9341.getCursorY();
  if (y > height)
  {
    display9341.fillScreen(ILI9341_BLACK);
    display9341.setCursor(x_pos, y_pos);
  }
  else
    display9341.setCursor(x_pos, y);
  display9341.println(ip);
}

// initialisiert das Display und zeigt das CANguru an
void initDisplayLCD28()
{
  const uint8_t splash_x = 48;
  const uint8_t splash_y = 48;
  bfillRect = false;
  display9341.begin();
  Wire.begin();
  pinMode(TFT_DC, OUTPUT);
  delay(500);
  ts.begin(TS_I2C_ADDRESS);
  delay(500);
  // Clear the buffer
  display9341.fillScreen(ILI9341_BLACK);
  display9341.setRotation(3);
  height = display9341.height();
  middleY = height / 2;
  middleX = display9341.width() / 2;
  uint8_t offset_x = (display9341.width() - display9341.width() / splash_x * splash_x) / 2;
  for (uint8_t sply = 0; sply < display9341.height() / splash_y; sply++)
    for (uint8_t splx = 0; splx < display9341.width() / splash_x; splx++)
      display9341.drawRGBBitmap(offset_x + splx * splash_x, sply * splash_y, image_data_CANguru, splash_x, splash_y);
  delay(1000);
  display9341.setTextColor(ILI9341_RED); // Draw red text
  display9341.setTextSize(2);            // Draw 2X-scale text
  displayLCD("CANguru-Bridge");
  displayLCD("CANguru 1.0");
  displayLCD("");
  display9341.setTextColor(ILI9341_WHITE); // Draw white text
}
#endif