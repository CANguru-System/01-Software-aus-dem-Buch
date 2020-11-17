/*

*/

#ifndef CANguruDefs
#define CANguruDefs

// allgemein
#define wait_time_long	  500
#define wait_time_medium	50
#define wait_time_small	  10
#define wait_time   		  125
#define max_char		      50
#define min_char	    	  2
#define min_char1	    	  3
#define bdrMonitor	      115200


/*
 *  Gerätetypen
 */
  /*
  Prio	2+2 Bit Message Prio			28 .. 25
										XXXX
  Priorität (Prio)
  Bestimmt die Priorisierung der Meldung auf dem CAN Bus:
  Prio 1: Stopp / Go / Kurzschluss-Meldung
  --> Prio 2: Rückmeldungen				0001
  Prio 3: Lok anhalten (?)
  --> Prio 4: Lok / Zubehörbefehle		0100
                                       Gleisbox
                                       0100 0111  0100 0100  0001 1001  0001 1000
                                          4    7     4    4     1    9     1    8
                                       Weichendecoder
                                          4    5     0    0     9    1     9    1

  Rest Frei								1111
*/

#define UID_BASE  0x45009195ULL    //CAN-UID
#define maxdevice 99

#define DEVTYPE_GFP       0x0000
#define DEVTYPE_GB        0x0010
#define DEVTYPE_CONNECT   0x0020
#define DEVTYPE_MS2       0x0030
#define DEVTYPE_WDEV      0x00E0
#define DEVTYPE_CS2       0x00FF
#define DEVTYPE_FirstCANguru  0x004F
#define DEVTYPE_BASE      0x0050
#define DEVTYPE_SERVO     0x0053
#define DEVTYPE_RM        0x0054
#define DEVTYPE_LIGHT     0x0055
#define DEVTYPE_SIGNAL    0x0056
#define DEVTYPE_LEDSIGNAL 0x0057
#define DEVTYPE_CANFUSE   0x0058
#define DEVTYPE_LastCANguru  0x005F

#define BASE_Offset      0x01
#define DECODER_Offset   0x02

/*
 * Adressbereiche:
*/
#define MM_ACC 		0x3000	  //Magnetartikel Motorola
#define DCC_ACC 	0x3800	  //Magbetartikel NRMA_DCC
#define MM_TRACK 	0x0000	  //Gleissignal Motorola
#define DCC_TRACK 0xC000    //Gleissignal NRMA_DCC

/*
 * CAN-Befehle (Märklin)
*/
#define SYS_STAT	          0x0B	//System - Status (sendet geänderte Konfiguration)
#define SWITCH_ACC 	        0x16	//Magnetartikel schalten
#define SYS_CMD		          0x00 	//Systembefehle
#define PING 		            0x30	//CAN-Teilnehmer anpingen
#define CONFIG_Status       0x3A

#define stopp               0x00
#define halt                0x02
#define overload            0x0A

/*
 * CAN-Befehle (eigene)
*/
#define ConfigData        0x40
#define ConfigData_R      0x41
#define MfxProc           0x50	
#define MfxProc_R         0x51	
#define BlinkAlive        0x60

// damit 1024 (eigentlich 1023) Artikel adressiert werden können,
// bedarf es 1024/4 Decoder; das 256 bzw. bei 255 entspr. 0xFF
const int16_t minadr = 0x01;
const int16_t maxadr = 0xFF;
const uint8_t uid_num = 4;

struct deviceparams
{
  uint8_t decoderadr;
  uint8_t uid_device[uid_num];
};

// converts highbyte of integer to char
char highbyte2char(int num);
// converts lowbyte of integer to char
char lowbyte2char(int num);
// returns one char of an int
uint8_t oneChar(uint16_t val, uint8_t no);

uint8_t hex2dec(uint8_t h);

#endif