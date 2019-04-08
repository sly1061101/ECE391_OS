#include "idt.h"
#include "lib.h"
#include "interrupt_linkage.h"
#include "syscall.h"

#define SYS_CALL 0x80
#define HALT_STATUS_ON_EXCEPTION 256

// Jump table for all interrupt hanlders. Last one is default handler.
void (*interrupt_handler[NUM_VEC + 1])();

/* format a macro to handle all different excpetions */
#define exception(name,message) \
void name() {                   \
    cli();                      \
    printf("%s\n", message);    \
    sti();                      \
    halt_current_process(HALT_STATUS_ON_EXCEPTION);  \
}

exception(exc_de,"Divide Error Exception");
exception(exc_db,"Debug Exception");
exception(exc_nmi,"NMI Interrupt");
exception(exc_bp,"Breakpoint Exception");
exception(exc_of,"Overflow Exception");
exception(exc_br,"BOUND Range Exceeded Exception");
exception(exc_ud,"Invalid Opcode Exception");
exception(exc_nm,"Device Not Available Exception");
exception(exc_df,"Double Fault Exception");
exception(exc_cso,"Coprocessor Segment Overrun");
exception(exc_ts,"Invalid TSS Exception");
exception(exc_np,"Segment Not Present");
exception(exc_ss,"Stack Full Exception");
exception(exc_gp,"General Protection Exception");

void exc_pf() {
    cli();
    uint32_t address;
    asm volatile("movl %%cr2, %0" \
                 :"=r"(address)   \
                 :                \
                 :"memory");
    printf("Page-Fault Exception. Address accessed: 0x%#x\n", address);
    sti();
    halt_current_process(HALT_STATUS_ON_EXCEPTION);
}

exception(exc_15,"INT 15 Handler!"); // Not exist in Intel manual.
exception(exc_mf,"x87 FPU Floating_Point Error");
exception(exc_ac,"Alignment Check Exception");
exception(exc_mc,"Machine-Check Exception");
exception(exc_xf,"SIMD Floating-Point Exception");

exception(default_handler,"Default interrupt was invoked!");

/* idt_init
 * 
 * Initialize IDT by populating different exceptions
 * Inputs: None
 * Outputs: None
 * Side Effects: Modify IDT
 */
void idt_init(){

    int i;

    for(i=0;i<NUM_VEC;i++){

        idt[i].present = 1;

        idt[i].dpl = 0; // hardware interrupt handlers and exception handlers should have DPL=0

        if(i==SYS_CALL){

          idt[i].dpl= 3; // system call handler has its DPL set to 3

        }

        idt[i].reserved0 = 0;
        idt[i].reserved1 = 1;
        idt[i].reserved2 = 1;
        idt[i].reserved3 = 1;  // load a descriptor with reserved bit
        idt[i].reserved4 = 0;

        idt[i].size = 1;  // 1 for 32 bit and 0 for 16 bit, set this file to 1 always

        if(i>=32){
           idt[i].reserved3 = 0; 
        }

        idt[i].seg_selector = KERNEL_CS;
    }

    // Populate interrupt handler jump table.
    interrupt_handler[0] = exc_de;
    interrupt_handler[1] = exc_db;
    interrupt_handler[2] = exc_nmi;
    interrupt_handler[3] = exc_bp;
    interrupt_handler[4] = exc_of;
    interrupt_handler[5] = exc_br;
    interrupt_handler[6] = exc_ud;
    interrupt_handler[7] = exc_nm;
    interrupt_handler[8] = exc_df;
    interrupt_handler[9] = exc_cso;
    interrupt_handler[10] = exc_ts;
    interrupt_handler[11] = exc_np;
    interrupt_handler[12] = exc_ss;
    interrupt_handler[13] = exc_gp;
    interrupt_handler[14] = exc_pf;
    interrupt_handler[15] = exc_15;
    interrupt_handler[16] = exc_mf;
    interrupt_handler[17] = exc_ac;
    interrupt_handler[18] = exc_mc;
    interrupt_handler[19] = exc_xf;

    interrupt_handler[NUM_VEC] = default_handler;

    // set up IDT entry
    for(i = 0; i < NUM_VEC; ++i) {
        SET_IDT_ENTRY(idt[i], ir_linkage_default);
    }

    SET_IDT_ENTRY(idt[0],ir_linkage_0);
    SET_IDT_ENTRY(idt[1],ir_linkage_1);
    SET_IDT_ENTRY(idt[2],ir_linkage_2);
    SET_IDT_ENTRY(idt[3],ir_linkage_3);
    SET_IDT_ENTRY(idt[4],ir_linkage_4);
    SET_IDT_ENTRY(idt[5],ir_linkage_5);
    SET_IDT_ENTRY(idt[6],ir_linkage_6);
    SET_IDT_ENTRY(idt[7],ir_linkage_7);
    SET_IDT_ENTRY(idt[8],ir_linkage_8);
    SET_IDT_ENTRY(idt[9],ir_linkage_9);
    SET_IDT_ENTRY(idt[10],ir_linkage_10);
    SET_IDT_ENTRY(idt[11],ir_linkage_11);
    SET_IDT_ENTRY(idt[12],ir_linkage_12);
    SET_IDT_ENTRY(idt[13],ir_linkage_13);
    SET_IDT_ENTRY(idt[14],ir_linkage_14);
    SET_IDT_ENTRY(idt[15],ir_linkage_15);
    SET_IDT_ENTRY(idt[16],ir_linkage_16);
    SET_IDT_ENTRY(idt[17],ir_linkage_17);
    SET_IDT_ENTRY(idt[18],ir_linkage_18);
    SET_IDT_ENTRY(idt[19],ir_linkage_19);
    SET_IDT_ENTRY(idt[33],ir_linkage_33);
    SET_IDT_ENTRY(idt[40],ir_linkage_40);
    SET_IDT_ENTRY(idt[SYS_CALL],syscall_linkage);
}
