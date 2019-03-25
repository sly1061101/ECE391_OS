#ifndef _RTC_H
#define _RTC_H

#include "types.h"

void rtc_handler(void); // rtc handler
void rtc_init(void); //rtc initialization
void set_freq(int32_t freq); // rtc set freq
int32_t rtc_open(const uint8_t* filename);
int32_t rtc_close(int32_t fd);
int32_t rtc_read(int32_t fd,void*buf,int32_t nbytes);
int32_t write(int32_t fd,const void*buf,int32_t nbytes);
#endif
