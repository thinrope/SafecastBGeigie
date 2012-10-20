/*
   The bGeigie-mini
   A device for car-borne radiation measurement (aka Radiation War-driving).

   Copyright (c) 2011, Robin Scheibler aka FakuFaku, Christopher Wang aka Akiba
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
   DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
   ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <SD.h>
#include <chibi.h>
#include <limits.h>
#include <EEPROM.h>
#include <GPS.h>
#include <InterruptCounter.h>
#include <math.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>

// compile time options, before #include "bGeigieMini.h"
#define GPS_MTK 1
#define GPS_CANMORE 2
#ifndef __RELEASE_NAME
	#define __RELEASE_NAME "1.x.y_abcdef"
#endif
#ifndef __VERSION_VERBOSE
	#define __VERSION_VERBOSE "unset [something is wrong]"
#endif
#ifndef ENABLE_DIAGNOSTIC
	#define ENABLE_DIAGNOSTIC 0
#endif
#ifndef PLUSSHIELD
	#define PLUSSHIELD 0
#endif
#ifndef JAPAN_POST
	#define JAPAN_POST 0
#endif
#ifndef TX_ENABLED
	#define TX_ENABLED 1
#endif
#ifndef GPS_PROGRAMMING
	#define GPS_PROGRAMMING 1
#endif
#ifndef GPS_TYPE
	#define GPS_TYPE GPS_MTK
#endif

#include "bGeigieMini.h"

#define TIME_INTERVAL 5000
#define NX 12
#define DEST_ADDR 0xFFFF      // this is the 802.15.4 broadcast address
#define CHANNEL 20
#define LED_ENABLED 0
#define PRINT_BUFSZ 80
#define AVAILABLE 'A'          // indicates geiger data are ready (available)
#define VOID      'V'          // indicates geiger data not ready (void)
#define BMRDD_EEPROM_ID 100
#define BMRDD_ID_LEN 3



// defines for status bits
#define SET_STAT(var, pos) var |= pos
#define UNSET_STAT(var, pos) var &= ~pos
#define CHECK_STAT(var, pos) (var & pos)
#define RAD_STAT 1
#define GPS_STAT 2
#define SD_STAT 4
#define WR_STAT 8

static const int chipSelect = 10;	// SD card select
static const int radioSelect = A3;
static const int sdPwr = 4;

// SD card detect and write protect input pins
static const int sd_cd = 6;
static const int sd_wp = 7;

#if ENABLE_DIAGNOSTIC
// We read voltage on analog pins 0 and 1
static const int pinV0 = A0;
static const int pinV1 = A1;
#endif

#if PLUSSHIELD
// pin to enable boost converter of LiPo battery
static const int pinBoost = 2;
#endif

unsigned long shift_reg[NX] = {0};
unsigned long reg_index = 0;
unsigned long total_count = 0;
int str_count = 0;
char geiger_status = VOID;

// This is the data file object that we'll use to access the data file
File dataFile; 

// the line buffer for serial receive and send
static char line[LINE_SZ];

const prog_char msg0[] PROGMEM = {"# NEW LOG\n# firmware="};
const prog_char msg1[] PROGMEM = {"SD init...\n"};
const prog_char msg2[] PROGMEM = {"Card failure...\n"};
const prog_char msg3[] PROGMEM = {"Card initialized\n"};
const prog_char msg4[] PROGMEM = {"Error: Log file cannot be opened.\n"};
const prog_char msg5[] PROGMEM = {"Device Id: "};
const prog_char msg6[] PROGMEM = {__RELEASE_NAME};		// about 16 bytes
const prog_char msg7[] PROGMEM = {__VERSION_VERBOSE};	// can be quite long, now 128 bytes
#define MAX_BUFF 32

char filename[13];      // placeholder for filename
char hdr[6] = "BMRDD";  // header for sentence
char dev_id[BMRDD_ID_LEN+1];  // device id
char ext_log[] = ".log";
char ext_bak[] = ".bak";



// Status vector
// 8bits
// 7: Reserved
// 6: Reserved
// 5: Reserved
// 4: Reserved
// 3: last write to SD card successful
// 2: SD card operational
// 1: GPS device operational
// 0: Radiation averaging status
static byte status = 0;
 
// State variables
int gps_init_acq = 0;
int log_created = 0;

/**************************************************************************/
/*!

*/
/**************************************************************************/
void setup()
{
  char tmp[MAX_BUFF];

  // init serial
  Serial.begin(9600);

  // enable and reset the watchdog timer
  wdt_enable(WDTO_8S);    // the arduino will automatically reset if it hangs for more than 8s
  wdt_reset();
  
  // initialize and program the GPS module
#if GPS_PROGRAMMING
  gps_program_settings();
#endif

  // print header to serial
  strcpy_P(tmp, msg0); Serial.print(tmp);
  strcpy_P(tmp, msg6); Serial.println(tmp);

  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(chipSelect, OUTPUT);
  digitalWrite(chipSelect, HIGH);     // disable SD card chip select 
  
  pinMode(radioSelect, OUTPUT);
  digitalWrite(radioSelect, HIGH);    // disable radio chip select
  
  pinMode(sdPwr, OUTPUT);
  digitalWrite(sdPwr, LOW);           // turn on SD card power

#if ENABLE_DIAGNOSTIC
  // setup analog reference to read battery and boost voltage
  analogReference(INTERNAL);
#endif

#if PLUSSHIELD
  // setup pin for boost converter
  pinMode(pinBoost, OUTPUT);
  digitalWrite(pinBoost, LOW);
  delay(20);
  pinMode(pinBoost, INPUT);
#endif
  
  // Create pulse counter on INT1
  interruptCounterSetup(1, TIME_INTERVAL);

  // init GPS using default Serial connection
  gps_init(&Serial, line);
  
  // set device id
  pullDevId();

  strcpy_P(tmp, msg5);
  Serial.print(tmp);
  Serial.println(dev_id);

  // print free RAM. we should try to maintain about 300 bytes for stack space
  Serial.println(FreeRam());

#if TX_ENABLED
  // init chibi on channel normally 20
  chibiInit();
  chibiSetChannel(CHANNEL);
  unsigned int addr = getRadioAddr();
  Serial.print("Addr: 0x");
  Serial.println(addr, HEX);
  chibiSetShortAddr(addr);
#endif

  strcpy_P(tmp, msg1);
  Serial.print(tmp);

  // setup card detect and write protect
  /*
  pinMode(sd_cd, INPUT);
  pinMode(sd_wp, INPUT);
  Serial.print("Card detect = ");
  Serial.println(digitalRead(sd_cd));
  Serial.print("Write protect = ");
  Serial.println(digitalRead(sd_wp));
  */
  
  // see if the card is present and can be initialized:
  if (SD.begin(chipSelect))
    SET_STAT(status, SD_STAT);
  if (!CHECK_STAT(status, SD_STAT))
  {
    strcpy_P(tmp, msg2);
    Serial.println(tmp);
  } else {
    strcpy_P(tmp, msg3);
    Serial.print(tmp);
  }

  // set initial state of Geiger to void
  geiger_status = VOID;
  str_count = 0;
  
  // print free RAM. we should try to maintain about 300 bytes for stack space
  Serial.println(FreeRam());

  // And now Start the Pulse Counter!
  interruptCounterReset();

  // Starting now!
  Serial.println("Starting now!");
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
void loop()
{
  char tmp[25];

  // update gps
  gps_update();

  // generate CPM every TIME_INTERVAL seconds
  if (interruptCounterAvailable())
  {
#if PLUSSHIELD
    // give a pulse to enable boost converter of LiPo pack
    pinMode(pinBoost, OUTPUT);
    digitalWrite(pinBoost, LOW);
    delay(20);
    pinMode(pinBoost, INPUT);
#endif
        
    // first, reset the watchdog timer
    wdt_reset();

    if (gps_available())
    {
      unsigned long cpm=0, cpb=0;
      byte line_len;

      // Check the SD card status and try to start it
      // if it failed previously
      if (!CHECK_STAT(status, SD_STAT))
      {
        strcpy_P(tmp, msg1);
        Serial.print(tmp);
        if (SD.begin(chipSelect)) 
          SET_STAT(status, SD_STAT);
        if (CHECK_STAT(status, SD_STAT))
        {
          strcpy_P(tmp, msg2);
          Serial.print(tmp);
        } else {
          strcpy_P(tmp, msg3);
          Serial.print(tmp);
        }
      }

      // obtain the count in the last bin
      cpb = interruptCounterCount();

      // reset the pulse counter
      interruptCounterReset();

      // insert count in sliding window and compute CPM
      shift_reg[reg_index] = cpb;     // put the count in the correct bin
      reg_index = (reg_index+1) % NX; // increment register index
      cpm = cpm_gen();                // compute sum over all bins

      // update the total counter
      total_count += cpb;
      
      // set status of Geiger
      if (str_count < NX)
      {
        geiger_status = VOID;
        str_count++;
      } else if (cpm == 0) {
        geiger_status = VOID;
      } else {
        geiger_status = AVAILABLE;
      }
      
      // generate timestamp. only update the start time if 
      // we printed the timestamp. otherwise, the GPS is still 
      // updating so wait until its finished and generate timestamp
      memset(line, 0, LINE_SZ);

      line_len = gps_gen_timestamp(line, shift_reg[reg_index], cpm, cpb);

      if (gps_init_acq == 0 && gps_getData()->status[0] == AVAILABLE)
      {
        // flag GPS acquired
        gps_init_acq = 1;
        // Create the filename for that drive
        strcpy(filename, dev_id);
        strcat(filename, "-");
        strncat(filename, gps_getData()->date+2, 2);
        strncat(filename, gps_getData()->date, 2);
        // print some comment line to mark beginning of new log
        strcpy(filename+8, ext_log);
        dataFile = SD.open(filename, FILE_WRITE);
        if (dataFile)
        {
          char msg_buff[MAX_BUFF];
          Serial.print("Filename: "); Serial.println(filename);

          dataFile.print("\n");
          strcpy_P(msg_buff, msg0); dataFile.print(msg_buff);
          strcpy_P(msg_buff, msg6); dataFile.println(msg_buff);
          dataFile.close();
        }
        else
        {
          char tmp[40];
          strcpy_P(tmp, msg4); Serial.print(tmp);
        }
        // write to backup file as well
        strcpy(filename+8, ext_bak);
        dataFile = SD.open(filename, FILE_WRITE);
        if (dataFile)
        {
          char msg_buff[MAX_BUFF];
          dataFile.print("\n");
          strcpy_P(msg_buff, msg0); dataFile.print(msg_buff);
          strcpy_P(msg_buff, msg6); dataFile.println(msg_buff);
          dataFile.close();
        }
        else
        {
          char tmp[40];
          strcpy_P(tmp, msg4);
          Serial.print(tmp);
        }
      }
      
      // turn on SD card power and delay a bit to initialize
      //digitalWrite(sdPwr, LOW);
      //delay(10);
      
      // init the SD card see if the card is present and can be initialized:
      //while (!SD.begin(chipSelect));

      if (gps_init_acq == 0)
      {
        Serial.print("No GPS: ");
        Serial.println(line);
#if TX_ENABLED
        // send out wirelessly. first wake up the radio, do the transmit, then go back to sleep
        chibiSleepRadio(0);
        delay(10);
        chibiTx(DEST_ADDR, (byte *)line, LINE_SZ);
        chibiSleepRadio(1);
#endif
      }
      else
      {
        // dump data to SD card
        strcpy(filename+8, ext_log);
        dataFile = SD.open(filename, FILE_WRITE);
        if (dataFile)
        {
          dataFile.print(line);
          dataFile.print("\n");

          dataFile.close();

        }
        else
        {
          char tmp[40];
          strcpy_P(tmp, msg4);
          Serial.print(tmp);
        }   
        
        // write to backup file as well
        write_to_file(ext_bak, line);

        // Printout line
        Serial.println(line);

#if TX_ENABLED
        // send out wirelessly. first wake up the radio, do the transmit, then go back to sleep
        chibiSleepRadio(0);
        delay(10);
        chibiTx(DEST_ADDR, (byte *)line, LINE_SZ);
        chibiSleepRadio(1);
#endif


#if ENABLE_DIAGNOSTIC
        // print voltage to BAK file
        int v0 = (int)(1000*read_voltage(pinV0));
        int v1 = (int)(1000*read_voltage(pinV1));
        sprintf(line, "#%d,%d", v0, v1);
        write_to_file(ext_bak, line);
        Serial.println(line);
#endif
        
        //turn off sd power        
        //digitalWrite(sdPwr, HIGH); 
      }
    }

  }
}

/* write a line to filename (global variable) with given extension to SD card */
void write_to_file(char *ext, char *line)
{
  // write to backup file as well
  strcpy(filename+8, ext);
  dataFile = SD.open(filename, FILE_WRITE);
  if (dataFile)
  {
    dataFile.print(line);
    dataFile.print("\n");
    dataFile.close();
  }
  else
  {
    char tmp[40];
    strcpy_P(tmp, msg4);
    Serial.print(tmp);
  }
}

/**************************************************************************/
byte gps_gen_timestamp(char *buf, unsigned long counts, unsigned long cpm, unsigned long cpb)
{
  byte len;
  byte chk;

  gps_t *ptr = gps_getData();

#if JAPAN_POST
  // if for JP, make sure to truncate the GPS coordinates
  truncate_JP(ptr->lat, ptr->lon);
#endif
  
  memset(buf, 0, LINE_SZ);
  sprintf_P(buf, PSTR("$%s,%s,20%s-%s-%sT%s:%s:%sZ,%ld,%ld,%ld,%c,%s,%s,%s,%s,%s,%s,%s,%s"),  \
              hdr, \
              dev_id, \
              ptr->datetime.year, ptr->datetime.month, ptr->datetime.day,  \
              ptr->datetime.hour, ptr->datetime.minute, ptr->datetime.second, \
              cpm, \
              cpb, \
              total_count, \
              geiger_status, \
              ptr->lat, ptr->lat_hem, \
              ptr->lon, ptr->lon_hem, \
              ptr->altitude, \
              ptr->status, \
              ptr->precision, \
              ptr->quality);
   len = strlen(buf);
   buf[len] = '\0';

   // generate checksum
   chk = gps_checksum(buf+1, len);

   // add checksum to end of line before sending
   if (chk < 16)
     sprintf(buf + len, "*0%X", (int)chk);
   else
     sprintf(buf + len, "*%X", (int)chk);

   return len;
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
unsigned long cpm_gen()
{
   unsigned int i;
   unsigned long c_p_m = 0;
   
   // sum up
   for (i=0 ; i < NX ; i++)
     c_p_m += shift_reg[i];
   
   return c_p_m;
}

void pullDevId()
{
  // counter for trials of reading EEPROM
  int n = 0;
  int N = 3;

  cli(); // disable all interrupts

  for (int i=0 ; i < BMRDD_ID_LEN ; i++)
  {
    // try to read one time
    dev_id[i] = (char)EEPROM.read(i+BMRDD_EEPROM_ID);
    n = 1;
    // while it's not numberic, and up to N times, try to reread.
    while ((dev_id[i] < '0' || dev_id[i] > '9') && n < N)
    {
      // wait a little before we retry
      delay(10);        
      // reread once and then increment the counter
      dev_id[i] = (char)EEPROM.read(i+BMRDD_EEPROM_ID);
      n++;
    }

    // catch when the read number is non-numeric, replace with capital X
    if (dev_id[i] < '0' || dev_id[i] > '9')
      dev_id[i] = 'X';
  }

  // set the end of string null
  dev_id[BMRDD_ID_LEN] = NULL;

  sei(); // re-enable all interrupts
}

/*
 * Set the address for the radio based on device ID
 */
unsigned int getRadioAddr()
{
  // prefix with two so that it never collides with broadcast address
  unsigned int addr = 0x2000;

  // first digit
  if (dev_id[0] != 'X')
    addr += (unsigned int)(dev_id[0]-'0') * 0x100;

  // second digit
  if (dev_id[1] != 'X')
    addr += (unsigned int)(dev_id[1]-'0') * 0x10;

  // third digit
  if (dev_id[2] != 'X')
    addr += (unsigned int)(dev_id[2]-'0') * 0x1;

  return addr;
}

#if JAPAN_POST
/* 
 * Truncate the latitude and longitude according to
 * Japan Post requirements
 * 
 * This algorithm truncate the minute
 * part of the latitude and longitude
 * in order to rasterize the points on
 * a 100x100m grid.
 */
void truncate_JP(char *lat, char *lon)
{
  unsigned long minutes;
  float lat0;
  unsigned int lon_trunc;

  /* latitude */
  // get minutes in one long int
  minutes =  (unsigned long)(lat[2]-'0')*100000 
    + (unsigned long)(lat[3]-'0')*10000 
    + (unsigned long)(lat[5]-'0')*1000 
    + (unsigned long)(lat[6]-'0')*100 
    + (unsigned long)(lat[7]-'0')*10 
    + (unsigned long)(lat[8]-'0');
  // truncate, for latutude, truncation factor is fixed
  minutes -= minutes % 546;
  // get this back in the string
  lat[2] = '0' + (minutes/100000);
  lat[3] = '0' + ((minutes%100000)/10000);
  lat[5] = '0' + ((minutes%10000)/1000);
  lat[6] = '0' + ((minutes%1000)/100);
  lat[7] = '0' + ((minutes%100)/10);
  lat[8] = '0' + (minutes%10);

  // compute the full latitude in radian
  lat0 = ((float)(lat[0]-'0')*10 + (lat[1]-'0') + (float)minutes/600000.f)/180.*M_PI;

  /* longitude */
  // get minutes in one long int
  minutes =  (unsigned long)(lon[3]-'0')*100000 
    + (unsigned long)(lon[4]-'0')*10000 
    + (unsigned long)(lon[6]-'0')*1000 
    + (unsigned long)(lon[7]-'0')*100 
    + (unsigned long)(lon[8]-'0')*10 
    + (unsigned long)(lon[9]-'0');
  // compute truncation factor
  lon_trunc = (unsigned int)((0.0545674090600784/cos(lat0))*10000.);
  // truncate
  minutes -= minutes % lon_trunc;
  // get this back in the string
  lon[3] = '0' + (minutes/100000);
  lon[4] = '0' + ((minutes%100000)/10000);
  lon[6] = '0' + ((minutes%10000)/1000);
  lon[7] = '0' + ((minutes%1000)/100);
  lon[8] = '0' + ((minutes%100)/10);
  lon[9] = '0' + (minutes%10);

}
#endif /* JAPAN_POST */


#if ENABLE_DIAGNOSTIC
float read_voltage(int pin)
{
  return 1.1*analogRead(pin)/1023. * 10.1;
}
#endif

#if GPS_PROGRAMMING
void gps_program_settings()
{
  // wait for GPS to start
  int t = 0;
  while(!Serial.available() && t < 5000)
  {
    delay(10);
    t += 10;
  }

#if GPS_TYPE == GPS_CANMORE

  // all GPS command taken from datasheet
  // "Binary Messages Of SkyTraq Venus 6 GPS Receiver"

  // set GGA and RMC output at 1Hz
  uint8_t GPS_MSG_OUTPUT_GGARMC_1S[9] = { 0x08, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01 }; // with update to RAM and FLASH
  uint16_t GPS_MSG_OUTPUT_GGARMC_1S_L = 9;

  // Power Save mode (not sure what it is doing at the moment
  uint8_t GPS_MSG_PWR_SAVE[3] = { 0x0C, 0x01, 0x01 }; // update to FLASH too
  uint16_t GPS_MSG_PWR_SAVE_L = 3;

  // send all commands
  gps_send_message(GPS_MSG_OUTPUT_GGARMC_1S, GPS_MSG_OUTPUT_GGARMC_1S_L);
  gps_send_message(GPS_MSG_PWR_SAVE, GPS_MSG_PWR_SAVE_L);

#elif GPS_TYPE == GPS_MTK

  char tmp[55];

  /* Set GPS output to NMEA GGA and RMC only at 1Hz */
  strcpy_P(tmp, PSTR(MTK_SET_NMEA_OUTPUT_RMCGGA));
  Serial.println(tmp);
  strcpy_P(tmp, PSTR(MTK_UPDATE_RATE_1HZ));
  Serial.println(tmp);

#endif /* GPS_TYPE */
}
#endif /* GPS_PROGAMMING */

#if GPS_PROGRAMMING && GPS_TYPE == GPS_CANMORE
void gps_send_message(const uint8_t *msg, uint16_t len)
{
  uint8_t chk = 0x0;

#if ARDUINO < 100
  // header
  Serial.print(0xA0, BYTE);
  Serial.print(0xA1, BYTE);
  // send length
  Serial.print(len >> 8, BYTE);
  Serial.print(len & 0xff, BYTE);
  // send message
  for (int i = 0 ; i < len ; i++)
  {
    Serial.print(msg[i], BYTE);
    chk ^= msg[i];
  }
  // checksum
  Serial.print(chk, BYTE);
  // end of message
  Serial.print(0x0D, BYTE);
  Serial.println(0x0A, BYTE);
#else
  // header
  Serial.write(0xA0);
  Serial.write(0xA1);
  // send length
  Serial.write(len >> 8);
  Serial.write(len & 0xff);
  // send message
  for (int i = 0 ; i < len ; i++)
  {
    Serial.write(msg[i]);
    chk ^= msg[i];
  }
  // checksum
  Serial.write(chk);
  // end of message
  Serial.write(0x0D);
  Serial.write(0x0A);
  Serial.write((byte)'\r');
  Serial.write((byte)'\n');
#endif
}
#endif /* GPS_CANMORE */

// vim: set tabstop=4 shiftwidth=4 syntax=c foldmethod=marker :
