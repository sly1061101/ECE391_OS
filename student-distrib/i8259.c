/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"


/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7  */
uint8_t slave_mask;  /* IRQs 8-15 */

/* Initialize the 8259 PIC */
void i8259_init(void) {
    outb(ALL_MASK,MASTER_8259_DATA)   /*mask all of 8258*/
    outb(ALL_MASK,SLAVE_8259_DATA)

    outb(ICW1,MASTER_8259_PORT)
    outb(ICW1,SLAVE_8259_PORT)

    outb(ICW2_MASTER ,MASTER_8259_DATA)
    outb(ICW2_MASTER ,SLAVE_8259_DATA)

    
    outb(ICW3_MASTER ,MASTER_8259_DATA)
    outb(ICW3_MASTER ,SLAVE_8259_DATA)

    outb(ICW4,MASTER_8259_DATA)   
    outb(ICW4,SLAVE_8259_DATA)

}

/* Enable (unmask) the specified IRQ */
void enable_irq(uint32_t irq_num) {
}

/* Disable (mask) the specified IRQ */
void disable_irq(uint32_t irq_num) {
}

/* Send end-of-interrupt signal for the specified IRQ */
void send_eoi(uint32_t irq_num) {
}
