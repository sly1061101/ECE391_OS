#include "rtc.h"
#include "i8259.h"
#include "types.h"
#include "lib.h"
#include "idt.h"
#include "keyboard.h"

#define RTC_REG_A 0x8A
#define RTC_REG_B 0x8B
#define RTC_REG_C 0x0C
#define RTC_REG_PORT 0x70
#define RTC_REG_DATA 0x71
#define RTC_RAM 0x20
#define ORED 0x40
#define RATE_OFFSET 0xF0
#define IRQ8 8

volatile int is_interrupt;
int rtc_counter = 0;
//void rtc_handler(void);

/*reference from https://wiki.osdev.org/RTC */

/*rtc_init
* DISCRIPTION: Initialize the rtc driver
* INPUT: NONE
* OUTPUT: NONE
* RETURN VALUE: NONE
* SIDE EFFECTS: rtc is initialized'
*/
void rtc_init(void)
{
    unsigned rate = 3;
    cli();
    is_interrupt = 0;

    /*setting the register*/
    outb(RTC_REG_A , RTC_REG_PORT);     // select status register a
    outb(RTC_RAM , RTC_REG_DATA); // write to cmos/rtc ram

    /*turn on irq8*/
    outb(RTC_REG_B , RTC_REG_PORT);     // select status register b
    char prev = inb(RTC_REG_DATA);
    outb(RTC_REG_B , RTC_REG_PORT);     // select status register b
    outb(prev | ORED, RTC_REG_DATA);

    /*change the interupt rate*/
    outb(RTC_REG_A,RTC_REG_PORT);		// set index to register A
    prev=inb(RTC_REG_DATA);	    // get initial value of register A
    outb(RTC_REG_A,RTC_REG_PORT);		// reset index to A
    outb((prev & RATE_OFFSET) | rate, RTC_REG_DATA); //write only our rate to A. Note, rate is the bottom 4 bits.

    // Register handler.
    interrupt_handler[40] = rtc_handler;

    enable_irq(IRQ8);
    sti();

}


/* rtc_handler
 * rtc interrupt handler 
 * Inputs: None
 * Outputs: None
 * Side Effects: none
 */

void rtc_handler(void)
{
    cli();
    rtc_counter++;
    is_interrupt = 1;
    outb(RTC_REG_C,RTC_REG_PORT);	// select register C
    inb(RTC_REG_DATA);		// just throw away contents
    send_eoi(IRQ8);
    sti();
}

int32_t rtc_open (const uint8_t* filename){
    //change the rate:
    unsigned n_rate =2;//rate change to 2 herz
    cli();
    outb(RTC_REG_A,RTC_REG_PORT);
    char prev=inb(RTC_REG_DATA);
    outb(RTC_REG_A,RTC_REG_PORT);
    outb((prev&0xF0)|n_rate,RTC_REG_DATA);
    sti();
	//enable irq
	enable_irq(IRQ8);
	
	return 0;

}

int32_t rtc_close(int32_t fd)
{
    return 0;
}

int32_t rtc_read(int32_t fd,void*buf,int32_t nbytes)
{
    cli();
    is_interrupt = 0;
    sti();
	while(!is_interrupt);

	return 0;
}


int32_t rtc_write(int32_t fd,const void*buf,int32_t nbytes)
{
	cli();
    if(buf == NULL || nbytes != 4)
    {
        sti();
        return -1;
    }
    int32_t freq;
    int32_t * buffer = (int32_t*) buf;
    freq = *buffer;
    set_freq(&freq);
    if(freq == -1)
    {
        sti();
        return -1;
    }

    outb(RTC_REG_A,RTC_REG_PORT);
    char prev=inb(RTC_REG_DATA);
    outb(RTC_REG_A,RTC_REG_PORT);
    outb((prev&0xF0)|freq,RTC_REG_DATA);
    sti();

    return 0;
}


void set_freq(int32_t *freq)
{
    switch(*freq)
    {
      case VAL_1024:
                *freq = FREQ_1024;
                break;
      case VAL_512:
                *freq = FREQ_512;
                break;
      case VAL_256:
                *freq = FREQ_256;
                break;
      case VAL_128:
                *freq = FREQ_128;
                break;
      case VAL_64:
                *freq = FREQ_64;
                break;
      case VAL_32:
                *freq = FREQ_32;
                break;
      case VAL_16:
                *freq = FREQ_16;
                break;
      case VAL_8:
                *freq = FREQ_8;
                break;
      case VAL_4:
                *freq = FREQ_4;
                break;
      case VAL_2:
                *freq = FREQ_2;
                break;
      default:
                *freq = -1;
                break;
    }
}







