#ifndef _RTC_H
#define _RTC_H

#include "types.h"

void rtc_handler(void); // rtc handler
void rtc_init(void); //rtc initialization
void rtc_set_freq(int freq); // rtc set freq
int32_t rtc_open();
int32_t rtc_close();
int32_t rtc_read();
int32_t rtc_write();
#endif
