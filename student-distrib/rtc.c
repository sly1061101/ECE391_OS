#include "rtc.h"
#include "i8259.h"
#include "types.h"
#include "lib.h"

#define RTC_REG_A 0x8A
#define RTC_REG_B 0x8B
#define RTC_REG_C 0x0C
#define RTC_REG_PORT 0x70
#define RTC_REG_DATA 0x71
#define RTC_RAM 0x20
#define ORED 0x40
#define RATE_OFFSET 0xF0


/*reference from https://wiki.osdev.org/RTC */
void rtc_init(void)
{
    unsigned rate = 3;
    cli();
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

    enable_irq(8);
    sti();

}

void rtc_handler(void)
{
    cli();
    outb(RTC_REG_C,RTC_REG_PORT);	// select register C
    inb(RTC_REG_DATA);		// just throw away contents
    send_eoi(8);
    sti();
}