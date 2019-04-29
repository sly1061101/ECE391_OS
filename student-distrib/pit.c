#include "pit.h"
#include "i8259.h"
#include "process.h"
#include "x86_desc.h"
#include "idt.h"
#include "terminal.h"
#include "paging.h"
#include "syscall.h"

#define TOTAL_CLOCK_FREQ 1193182
#define PIT_COMMAND_REGISTER 0x43
#define PIT_COMMAND_BYTE 0x36
#define LSB_MASK  0xFF
#define HSB_SHIFT 8
#define CHANNEL_0 0x40
#define PIT_IRQ 	0

/*
 * 	 pit_init
 *   DESCRIPTION: Initialize the Programmable Interval Timer
 *   INPUTS: set frequency
 *   OUTPUTS: nond
 *   RETURN VALUE: none
 *   SIDE EFFECTS: pit that interrupts with given frequency
 */
void pit_init(int32_t freq){
	/* Calculate our divisor */
	int32_t divisor;
	divisor = TOTAL_CLOCK_FREQ / freq;

	/* Set our command byte 0x36 */
	outb(PIT_COMMAND_BYTE, PIT_COMMAND_REGISTER); 

	/* Set low byte of divisor */            
    outb(divisor & LSB_MASK, CHANNEL_0);

    /* Set high byte of divisor */  
    outb(divisor >> HSB_SHIFT, CHANNEL_0);  

    interrupt_handler[PIT_VEC_NUM] = pit_handler;

    enable_irq(PIT_IRQ);   
}

/*
 * 	 pit_handler
 *   DESCRIPTION: handle pit interrupt
 *   INPUTS: nond
 *   OUTPUTS: 
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void pit_handler(){
	send_eoi(PIT_IRQ);

    if(!is_scheduling_started())
        return;

    // If there are still inactive terminals, start shell on it.
    if(get_next_inactive_terminal() != -1) {
        // If already has process running, we need to backup some data for it so that we can switch back to it later.
        if(process_count > 0) {
            pcb_t *curr_pcb = get_current_pcb();

            // Save the current esp and ebp 
            asm volatile("movl %%esp, %0" \
                            :"=r"(curr_pcb->esp)   \
                            :                \
                            :"memory");

            asm volatile("movl %%ebp, %0" \
                            :"=r"(curr_pcb->ebp)   \
                            :                \
                            :"memory");
            
            // Save the screen cursor position.
            backup_screen_position(&screen_x_backstore[curr_pcb->terminal_id], &screen_y_backstore[curr_pcb->terminal_id]);
        }

        // Set the initial cursor for next shell to origin.
        load_screen_position(0, 0);
        (void) syscall_execute((uint8_t*)"shell");

        // We will never really reach here.
        return;
    }

    // If all terminals are active, switch to next scheduled process.
    switch_process(next_scheduled_process());
}
