
#include <WiFi.h>
#include <EEPROM.h>
#include <esp_now.h>
#include <Ticker.h>
#include "CANguruDefs.h"

// EEPROM-Adressen
#define  setup_done 0x47
// EEPROM-Belegung
// EEPROM-Speicherplätze der Local-IDs
const uint16_t adr_setup_done = 0x00;
const uint8_t adr_decoderadr = 0x01;
const uint16_t lastAdr = adr_decoderadr;
#define EEPROM_SIZE lastAdr+1

// config-Daten

enum Kanals {
  Kanal00, Kanal01, 
  endofKanals
};

Kanals CONFIGURATION_Status_Index = Kanal00;
bool CONFIG_Status_Request = false;
bool SYS_CMD_Request = false;
// Timer
boolean statusPING;

#define VERS_HIGH     0x00  // Versionsnummer vor dem Punkt
#define VERS_LOW      0x01  // Versionsnummer nach dem Punkt

/*
Variablen der Signals & Magnetartikel
*/
// Protokollkonstante
#define PROT  MM_ACC

#include "espnow.h"

uint8_t readValfromEEPROM(uint16_t adr, uint8_t val, uint8_t min, uint8_t max) {
  uint8_t v = EEPROM.read(adr);
  if ((v>=min) && (v<=max))
    return v;
  else
    return val;
}

void setup() {
  Serial.begin(115200);
  Serial.println("\r\n\r\nB a s i s d e c o d e r");
  startAPMode();
  addMaster();
  WiFi.disconnect();
  //
  // app starts here
  //
  EEPROM.begin(EEPROM_SIZE);
  uint8_t setup_todo = EEPROM.read(adr_setup_done);
  if (setup_todo != setup_done){
    // wurde das Setup bereits einmal durchgeführt?
    // dann wird dieser Anteil übersprungen
    // 47, weil das EEPROM (hoffentlich) nie ursprünglich diesen Inhalt hatte

    // setzt die Boardnum anfangs auf 1
    params.decoderadr = 1;
    EEPROM.write (adr_decoderadr, params.decoderadr);
    EEPROM.commit();

      // setup_done auf "TRUE" setzen
    EEPROM.write (adr_setup_done, setup_done);
    EEPROM.commit();
  }
  else {
    params.decoderadr = readValfromEEPROM(adr_decoderadr, minadr, minadr, maxadr);
  }
  // ab hier werden die Anweisungen bei jedem Start durchlaufen
  got1CANmsg = false;
  SYS_CMD_Request = false;
  statusPING = false;
  stillAliveBlinkSetup();
}

void sendCanFrame(){
  // to Server
  for (uint8_t i = CAN_FRAME_SIZE-1; i<8-opFrame[4]; i--)
    opFrame[i] = 0x00;
  opFrame[1]++;
  opFrame[2] = hasharr[0];
  opFrame[3] = hasharr[1];
  sendTheData();
}

bool testMinMax(uint8_t oldval, uint8_t val, uint8_t min, uint8_t max) {
  return (oldval!=val) && (val>=min) && (val<=max);
}

void receiveKanalData() {
  SYS_CMD_Request = false;
  uint8_t oldval;
  switch (opFrame[10]){
    // Kanalnummer #1 - Decoderadresse
    case 1: {
      oldval = params.decoderadr;
      params.decoderadr = (opFrame[11]<<8)+opFrame[12];
      if (testMinMax(oldval, params.decoderadr, minadr, maxadr)) {
        // speichert die neue Adresse
        EEPROM.write (adr_decoderadr, params.decoderadr);
        EEPROM.commit();
        // neue Adressen
      } else {
        params.decoderadr = oldval;
      }
    }
    break;
  }
  // antworten
  opFrame[11] = 0x01;
  opFrame[4] = 0x07;
  sendCanFrame();
}

void sendPING() {
  statusPING = false;
  opFrame[1] = PING;
  opFrame[4] = 0x08;
  for (uint8_t i = 0; i < uid_num; i++) {
    opFrame[i+5] = params.uid_device[i];
  }
  opFrame[9] = VERS_HIGH;
  opFrame[10] = VERS_LOW;
  opFrame[11] = DEVTYPE_BASE >> 8;
  opFrame[12] = DEVTYPE_BASE;
  sendCanFrame();
}

void sendConfig() {
  const uint8_t Kanalwidth = 8;
  const uint8_t numberofKanals = endofKanals-1;

  const uint8_t NumLinesKanal00 = 5*Kanalwidth;
  uint8_t arrKanal00[NumLinesKanal00] = {
    /*1*/    Kanal00, numberofKanals, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, (uint8_t) 0, params.decoderadr,
    /*2.1*/  ( uint8_t ) highbyte2char(hex2dec(params.uid_device[0])), ( uint8_t ) lowbyte2char(hex2dec(params.uid_device[0])),
    /*2.2*/  ( uint8_t ) highbyte2char(hex2dec(params.uid_device[1])), ( uint8_t ) lowbyte2char(hex2dec(params.uid_device[1])),
    /*2.3*/  ( uint8_t ) highbyte2char(hex2dec(params.uid_device[2])), ( uint8_t ) lowbyte2char(hex2dec(params.uid_device[2])),
    /*2.4*/  ( uint8_t ) highbyte2char(hex2dec(params.uid_device[3])), ( uint8_t ) lowbyte2char(hex2dec(params.uid_device[3])),
    /*3*/    'C', 'A', 'N', 'g', 'u', 'r', 'u', ' ',
    /*4*/    'B', 'a', 's','i', 's', 'd', 'e', 'c',
    /*5*/    'o', 'd', 'e', 'r', 0, 0, 0, 0
  };
  const uint8_t NumLinesKanal01 = 4*Kanalwidth;
  uint8_t arrKanal01[NumLinesKanal01] = {
    // #2 - WORD immer Big Endian, wie Uhrzeit
    /*1*/    Kanal01, 2, 0, minadr, 0, maxadr, 0, params.decoderadr,
    /*2*/    'M', 'o', 'd', 'u', 'l', 'a', 'd', 'r',
    /*3*/    'e', 's', 's', 'e', 0, '1', 0, (maxadr/100)+'0',
    /*4*/    (maxadr-(uint8_t)(maxadr/100)*100)/10+'0', (maxadr-(uint8_t)(maxadr/10)*10) +'0', 0, 'A', 'd', 'r', 0, 0
  };
  uint8_t NumKanalLines[numberofKanals+1] = {
    NumLinesKanal00, NumLinesKanal01
  };

  uint8_t paket = 0;
  uint8_t outzeichen = 0;
  CONFIG_Status_Request = false;
  for (uint8_t inzeichen = 0; inzeichen < NumKanalLines[CONFIGURATION_Status_Index]; inzeichen++) {
    opFrame[1] = CONFIG_Status+1;
    switch (CONFIGURATION_Status_Index)  {
      case Kanal00: {
        opFrame[outzeichen+5] = arrKanal00[inzeichen];
      }
      break;
      case Kanal01: {
        opFrame[outzeichen+5] = arrKanal01[inzeichen];
      }
      break;
      case endofKanals: {
        // der Vollständigkeit geschuldet
      }
      break;
    }
    outzeichen++;
    if (outzeichen==8) {
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
  opFrame[1] = CONFIG_Status+1;
  opFrame[2] = hasharr[0];
  opFrame[3] = hasharr[1];
  opFrame[4] = 0x06;
  for (uint8_t i = 0; i < 4; i++) {
    opFrame[i+5] = params.uid_device[i];
  }
  opFrame[9] = CONFIGURATION_Status_Index;
  opFrame[10] = paket;
  sendTheData();
  delay(wait_time_small);
}

void loop() {
  if (got1CANmsg) {
    got1CANmsg = false;
    if (statusPING) {
      sendPING();
    }
    if (SYS_CMD_Request) {
      receiveKanalData();
    }
    if (CONFIG_Status_Request) {
      sendConfig();
    }
  }
}
