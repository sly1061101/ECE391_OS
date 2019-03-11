/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"


/*reference from https://wiki.osdev.org/8259_PIC#Common_Definitions */
/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7  */
uint8_t slave_mask;  /* IRQs 8-15 */




/*wait_io
* DISCRIPTION: setup wait time
* INPUT: NONE
* OUTPUT: NONE
* RETURN VALUE: NONE
* SIDE EFFECTS: wait after outb'
*/
void wait_io() {
    int wait_time = 1000000;
    int i;
    for(i = 0; i < wait_time; ++i) ;
}
/*i8259_init
* DISCRIPTION: Initialize the 8259 PIC 
* INPUT: NONE
* OUTPUT: NONE
* RETURN VALUE: NONE
* SIDE EFFECTS: PIC is initialized'
*/
void i8259_init(void) {
    outb(ALL_MASK, MASTER_8259_DATA);   /*mask all of 8259*/
    outb(ALL_MASK, SLAVE_8259_DATA);
    wait_io();
    
    outb(ICW1, MASTER_8259_PORT);     /*init all port in cascade mode*/
    outb(ICW1, SLAVE_8259_PORT);
    wait_io();

    outb(ICW2_MASTER, MASTER_8259_DATA);   /*set the pic offset vector*/
    outb(ICW2_SLAVE, SLAVE_8259_DATA);
    wait_io();

    outb(ICW3_MASTER, MASTER_8259_DATA);  
    outb(ICW3_SLAVE, SLAVE_8259_DATA);
    wait_io();

    outb(ICW4, MASTER_8259_DATA);   
    outb(ICW4, SLAVE_8259_DATA);
    wait_io();

    master_mask = ALL_MASK;
    slave_mask = ALL_MASK;
}


/*enable_irq
* DISCRIPTION: Enable (unmask) the specified IRQ 
* INPUT: NONE
* OUTPUT: NONE
* RETURN VALUE: NONE
* SIDE EFFECTS: NONE
*/
void enable_irq(uint32_t irq_num) {
    uint8_t bit_mask = 1;
    if(irq_num<8) /*for master unmask*/
    {
        master_mask = master_mask & (~(bit_mask << irq_num));
        outb(master_mask,MASTER_8259_DATA);
    }
    else /*for slave unmask*/
    {
        irq_num = irq_num-8;
        slave_mask = slave_mask & (~(bit_mask << irq_num));
        outb(slave_mask,SLAVE_8259_DATA);
        master_mask =  master_mask & (~(bit_mask << 2));
        outb(master_mask,MASTER_8259_DATA);
    }
}



/*disable_irq
* DISCRIPTION: Disable (mask) the specified IRQ
* INPUT: NONE
* OUTPUT: NONE
* RETURN VALUE: NONE
* SIDE EFFECTS: NONE
*/
void disable_irq(uint32_t irq_num) {
    uint8_t bit_mask = 1;
    if(irq_num<8) /*for master mask*/
    {
        master_mask = master_mask & (bit_mask << irq_num);
        outb(master_mask,MASTER_8259_DATA);
    }
    else /*for slave mask*/
    {
        irq_num = irq_num-8;
        slave_mask = slave_mask & (bit_mask << irq_num);
        outb(slave_mask,SLAVE_8259_DATA);
    }
}

/*send_eoi
* DISCRIPTION: Send end-of-interrupt signal for the specified IRQ
* INPUT: NONE
* OUTPUT: NONE
* RETURN VALUE: NONE
* SIDE EFFECTS: NONE
*/
void send_eoi(uint32_t irq_num) {
    /*8-15 for slave*/
    if(irq_num >= 8)
    {
        irq_num-=OFFSET8;
        outb(EOI|irq_num,SLAVE_8259_PORT);   
        outb(OFFSET2|EOI, MASTER_8259_PORT);
    }
    /*0-7 for master*/
    else
    {
        outb(EOI|irq_num,MASTER_8259_PORT);
    }
}
