#include "rtc.h"
#include "i8259.h"
#include "types.h"
#include "lib.h"
#include "idt.h"
#include "keyboard.h"
#include "process.h"

#define RTC_REG_A 0x8A
#define RTC_REG_B 0x8B
#define RTC_REG_C 0x0C
#define RTC_REG_PORT 0x70
#define RTC_REG_DATA 0x71
#define RTC_RAM 0x20
#define ORED 0x40
#define RATE_OFFSET 0xF0
#define IRQ8 8
#define HIGH_FOUR_BITS_MASK 0xF0

#define PHYSICAL_RTC_FREQ 1024

int rtc_counter = 0;
//void rtc_handler(void);

uint32_t process_rtc_counter[MAX_PROCESS_NUMBER];
uint32_t process_counter_target[MAX_PROCESS_NUMBER];
volatile uint32_t process_is_interrupted[MAX_PROCESS_NUMBER];

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
    int32_t i;
    int32_t freq = PHYSICAL_RTC_FREQ;

    cli();
    
    for(i = 0; i < MAX_PROCESS_NUMBER; ++i) {
        process_rtc_counter[i] = 0;
        process_counter_target[i] = 0;
        process_is_interrupted[i] = 0;
    }
    rtc_counter = 0;

    set_freq(&freq);

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
    outb((prev & RATE_OFFSET) | freq, RTC_REG_DATA); //write only our rate to A. Note, rate is the bottom 4 bits.

    // Register handler.
    interrupt_handler[RTC_VEC_NUM] = rtc_handler;

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
    int32_t i;
    cli();
    rtc_counter++;
    for(i = 0; i < MAX_PROCESS_NUMBER; ++i) {
        if(process_counter_target[i] != 0) {
            process_rtc_counter[i]++;
            if(process_rtc_counter[i] == process_counter_target[i]) {
                process_is_interrupted[i] = 1;
                process_rtc_counter[i] = 0;
            }
        }
    }
    outb(RTC_REG_C,RTC_REG_PORT);	// select register C
    inb(RTC_REG_DATA);		// just throw away contents
    send_eoi(IRQ8);
    sti();
}

/*rtc_open
* DISCRIPTION: open rtc and initialize frequency to 2 herz
* INPUT: const uint8_t* filename
* OUTPUT: NONE
* RETURN VALUE: 0
* SIDE EFFECTS: rtc is open and frequency set to 2 herz.
*/

int32_t rtc_open (const uint8_t* filename){
    process_counter_target[get_current_pcb()->pid] = PHYSICAL_RTC_FREQ / VAL_2;
	return 0;
}

/*rtc_close
* DISCRIPTION: close rtc
* INPUT: int32_t fd
* OUTPUT: NONE
* RETURN VALUE: 0
* SIDE EFFECTS: rtc is closed.
*/

int32_t rtc_close(int32_t fd)
{
    process_rtc_counter[get_current_pcb()->pid] = 0;
    process_counter_target[get_current_pcb()->pid] = 0;
    process_is_interrupted[get_current_pcb()->pid] = 0;
    return 0;
}

/*rtc_read
* DISCRIPTION: rtc return 0 only after interrupt occurs
* INPUT:    void*buf
            int32_t fd
            int32_t nbytes
* OUTPUT: NONE
* RETURN VALUE: 0
* SIDE EFFECTS: rtc receives interrupts and returns 0.
*/

int32_t rtc_read(int32_t fd,void*buf,int32_t nbytes)
{
    cli();
    process_is_interrupted[get_current_pcb()->pid] = 0;
    sti();
	while(!process_is_interrupted[get_current_pcb()->pid]);

	return 0;
}

/*rtc_write
* DISCRIPTION: writes data to device or  terminal
* INPUT:    void*buf
            int32_t fd
            int32_t nbytes
* OUTPUT: NONE
* RETURN VALUE: 0
* SIDE EFFECTS: rtc writes data to device or terminal at certain rate(no more than 1024 herz)
*/

int32_t rtc_write(int32_t fd,const void*buf,int32_t nbytes)
{
	cli();
    if(buf == NULL || nbytes != VAL_4)
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

    freq = *buffer;
    process_counter_target[get_current_pcb()->pid] = PHYSICAL_RTC_FREQ / freq;

    return 0;
}   

/*set_freq
* DISCRIPTION: sets frequency value
* INPUT:    int32_t freq
* OUTPUT: NONE
* RETURN VALUE: NONE
* SIDE EFFECTS: sets frequency according to the parameter freq.
*/

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







