
/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <CANguru-Buch@web.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gustav Wostrack
 * ----------------------------------------------------------------------------
 */

#include <Arduino.h>
#include "CANguruDefs.h"

char highbyte2char(int num){
  num /= 10;
  return char ('0' + num);
}

char lowbyte2char(int num){
  num = num - num / 10 * 10;
  return char ('0' + num);
}

uint8_t oneChar(uint16_t val, uint8_t pos) {
	char buffer [5];
	itoa (val, buffer, 10);
	return buffer[4 - pos];
}

uint8_t dev_type = DEVTYPE_BASE;

uint8_t hex2dec(uint8_t h){
  return h / 16 * 10 + h % 16;
}

