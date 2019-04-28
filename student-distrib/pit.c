#include "pit.h"
#include "i8259.h"
#include "process.h"
#include "x86_desc.h"
#include "idt.h"
#include "terminal.h"
#include "paging.h"

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

    interrupt_handler[PIT_VEC_NUM] = (uint32_t)pit_handler;

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

	cli();
	send_eoi(PIT_IRQ);

	// // get next active terminal
	// int current_terminal = get_running_terminal();
	// int next_terminal = (current_terminal+1)%3;

	// get current pcb
    pcb_t * curr_pcb = get_current_pcb();

    // // Save the current esp and ebp 
    // uint32_t esp;
    // uint32_t ebp;

    if(get_next_inactive_terminal() != -1) {
        if(process_count > 0)
            backup_screen_position(&screen_x_backstore[curr_pcb->terminal_id], &screen_y_backstore[curr_pcb->terminal_id]);
        load_screen_position(0, 0);
        syscall_execute((uint8_t*)"shell");
        return;
    }

    // asm volatile("movl %%ebp, %0" \
    //              :"=r"(ebp)   \
    //              :                \
    //              :"memory");

    // // Switch paging
    // // TODO

    // // Modify TSS for context switch.
    // tss.ss0 = KERNEL_DS;
    // // TODO
    // tss.esp0 = curr_pcb->esp0;

	// // change esp and ebp
	// // TODO
	// asm volatile(
    //     "movl   %0, %%esp   ;"
    //     "movl   %1, %%ebp   ;"
    //     "LEAVE;"
    //     "RET;"
    //     : :"r"(next_pcb->current_esp), "r"(next_pcb->current_ebp) 
    // );
	sti();

}
