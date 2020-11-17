#ifndef ESP_NOW_H
#define ESP_NOW_H

#include <Arduino.h>

const uint8_t WIFI_CHANNEL = 0;
const uint8_t macLen = 6;

/* Im using the MAC addresses of my specific ESP32 modules, you will need to 
   modify the code for your own:
   The SLAVE Node needs to be initialized as a WIFI_AP
   you will need to get its MAC via WiFi.softAPmacAddress()
*/

enum statustype { even=0, odd };
statustype currStatus;

const uint8_t masterCustomMac[] = {0x30, 0xAE, 0xA4, 0x89, 0x92, 0x71};

esp_now_peer_info_t master;
const esp_now_peer_info_t *masterNode = &master;
uint8_t opFrame[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const uint8_t CAN_FRAME_SIZE = 13;		/* maximum frame size */
bool got1CANmsg = false;
byte cnt=0;
String ssid0 = "CNgrSLV";
deviceparams params;
uint8_t hasharr[] = { 0x00, 0x00 };

Ticker tckr;
const float tckrTime = 0.025; 

uint16_t secs;

//*********************************************************************************************************
void timer1s() {
  if (secs>0) {
  	if (secs % 2 == 0) {
      digitalWrite(LED_BUILTIN, LOW);
    }
    else {
      digitalWrite(LED_BUILTIN, HIGH);
    }
    secs--;
  }
  else {
    digitalWrite(LED_BUILTIN, LOW);
  }
}

void stillAliveBlinkSetup() {
  tckr.attach(tckrTime, timer1s);  // each sec
  // initialize LED digital pin as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  // turn the LED off by making the voltage LOW
  digitalWrite(LED_BUILTIN, LOW);
  secs = 0;
}

void generateHash(uint8_t offset) {  
  uint32_t uid = UID_BASE + offset;
  params.uid_device[0] = (uint8_t) (uid >> 24);
  params.uid_device[1] = (uint8_t) (uid >> 16);
  params.uid_device[2] = (uint8_t) (uid >> 8);
  params.uid_device[3] = (uint8_t) uid;

  uint16_t highbyte = uid >> 16;
  uint16_t lowbyte = uid;
  uint16_t hash = highbyte ^ lowbyte;
  bitWrite(hash, 7, 0);
  bitWrite(hash, 8, 1);
  bitWrite(hash, 9, 1);
  hasharr[0] = hash>>8;
  hasharr[1] = hash;
}

void startAPMode() {
  Serial.println("WIFI Connect AP Mode");
  WiFi.persistent(false); 
  WiFi.mode(WIFI_OFF); 
  WiFi.mode(WIFI_AP); 
  
  // Connect to Wi-Fi
  String ssid1 = WiFi.softAPmacAddress();
  ssid0 = ssid0 + ssid1;
  char ssid[30];
  ssid0.toCharArray(ssid, 30);
  WiFi.softAP(ssid);  // Name des Access Points
  Serial.println(ssid);
}

void printESPNowError(esp_err_t Result) {
  if (Result == ESP_ERR_ESPNOW_NOT_INIT) {
    // How did we get so far!!
    Serial.println("ESPNOW not Init.");
  } else if (Result == ESP_ERR_ESPNOW_ARG) {
    Serial.println("Invalid Argument");
  } else if (Result == ESP_ERR_ESPNOW_INTERNAL) {
    Serial.println("Internal Error");
  } else if (Result == ESP_ERR_ESPNOW_NO_MEM) {
    Serial.println("ESP_ERR_ESPNOW_NO_MEM");
  } else if (Result == ESP_ERR_ESPNOW_NOT_FOUND) {
    Serial.println("Peer not found.");
  } 
    else if (Result == ESP_ERR_ESPNOW_IF) {
    Serial.println("Interface Error.");
  }   else {
    Serial.printf("\r\nNot sure what happened\t%d", Result);
  }
}

void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  memcpy( opFrame, data, data_len );
  if (data_len == macLen + 1) {
    // nur beim handshaking; es werden die macaddress
    // und die devicenummer übermittelt und die macaddress zurückgeschickt
    esp_err_t sendResult = esp_now_send(master.peer_addr, opFrame, macLen);
    if (sendResult != ESP_OK) {
      printESPNowError(sendResult);
    }
    generateHash(opFrame[macLen]);
    return;
  }
  switch (opFrame[0x01]){
    case BlinkAlive:
    if (secs<10)
        secs = 10;
    break;
    case PING: {
      statusPING = true;
      got1CANmsg = true;
      // alles Weitere wird in loop erledigt
    }
    break;
    // config
    case CONFIG_Status: {
        CONFIG_Status_Request = true;
        CONFIGURATION_Status_Index = (Kanals) opFrame[9];
        if (CONFIGURATION_Status_Index>0 && secs<100)
          secs = 100;
        got1CANmsg = true;
    }
    break;
    case SYS_CMD: {
      if (opFrame[9]==SYS_STAT) {
        SYS_CMD_Request = true;
        // alles Weitere wird in loop erledigt
      got1CANmsg = true;
      }
    }
    break;
    case SWITCH_ACC:
    break;
  }
}

void sendTheData() {
  esp_now_send(master.peer_addr, opFrame, CAN_FRAME_SIZE);
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "OK" : "Fail");
}

void addMaster() {
  if(esp_now_init() == ESP_OK) {
    Serial.println("ESPNow Init Success!");
  }
  else {
    Serial.println("ESPNow Init Failed....");
  }
  esp_now_register_recv_cb(OnDataRecv);
  esp_now_register_send_cb(OnDataSent);
	memcpy( &master.peer_addr, &masterCustomMac, macLen );
  master.channel = WIFI_CHANNEL; // pick a channel
  master.encrypt = 0; // no encryption
  //Add the master node to this slave node
  master.ifidx = ESP_IF_WIFI_AP;
  //Add the remote master node to this slave node
	if( esp_now_add_peer(masterNode) == ESP_OK) {
		Serial.println("Master Added As Peer!");
	}
}

#endif