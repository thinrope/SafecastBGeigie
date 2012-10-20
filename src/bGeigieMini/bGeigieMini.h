#ifndef bGeigieMini_h
#define bGeigieMini_h

#include "Arduino.h"

void write_to_file(char *, char *);
byte gps_gen_timestamp(char *, unsigned long, unsigned long, unsigned long);
unsigned long cpm_gen(void);
void pullDevId(void);
unsigned int getRadioAddr(void);

#if JAPAN_POST
void truncate_JP(char *, char *);
#endif /* JAPAN_POST */


#if ENABLE_DIAGNOSTIC
float read_voltage(int);
#endif

#if GPS_PROGRAMMING
void gps_program_settings(void);
#endif /* GPS_PROGAMMING */

#if GPS_PROGRAMMING && GPS_TYPE == GPS_CANMORE
void gps_send_message(const uint8_t *, uint16_t);
#endif /* GPS_CANMORE */


#endif /* bGeigieMini_h */

// vim: set tabstop=4 shiftwidth=4 syntax=c foldmethod=marker :
