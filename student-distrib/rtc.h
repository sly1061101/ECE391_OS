#ifndef _RTC_H
#define _RTC_H




#define VAL_2       2
#define VAL_4       4
#define VAL_8       8
#define VAL_16      16
#define VAL_32      32
#define VAL_64      64
#define VAL_128     128
#define VAL_256     256
#define VAL_512     512
#define VAL_1024    1024

#define FREQ_2      0xF
#define FREQ_4      0xE
#define FREQ_8      0xD
#define FREQ_16     0xC
#define FREQ_32     0xB
#define FREQ_64     0xA
#define FREQ_128    0x9
#define FREQ_256    0x8
#define FREQ_512    0x7
#define FREQ_1024   0x6




#include "types.h"

void rtc_handler(void); // rtc handler
void rtc_init(void); //rtc initialization
void set_freq(int32_t freq); // rtc set freq
int32_t rtc_open(const uint8_t* filename);
int32_t rtc_close(int32_t fd);
int32_t rtc_read(int32_t fd,void*buf,int32_t nbytes);
int32_t rtc_write(int32_t fd,const void*buf,int32_t nbytes);

#endif
