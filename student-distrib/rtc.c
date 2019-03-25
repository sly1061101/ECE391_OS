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
    outb(RTC_REG_C,RTC_REG_PORT);	// select register C
    inb(RTC_REG_DATA);		// just throw away contents
    send_eoi(IRQ8);
    sti();
}
