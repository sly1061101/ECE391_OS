#include "idt.h"
#include "x86_desc.h"
#include "lib.h"


#define SYS_CALL 0x80

/* format a macro to handle all different excpetions */
#define exception(name,message) \
void name() {  \
    cli();      \
    printf("%s\n", #message);   \
    while(1);   \
}

exception(exc_de,"0. Divide Error Exception");
exception(exc_db,"Debug Exception");
exception(exc_nmi,"NMI Interrupt");
exception(exc_bp,"Breakpoint Exception");
exception(exc_of,"Overflow Exception");
exception(exc_br,"BOUND Range Exceeded Exception");
exception(exc_ud,"Invalid Opcode Exception");
exception(exc_nm,"Device Not Available Exception");
exception(exc_df,"Doublt Fault Exception");
exception(exc_cso,"Coprocessor Segment Overrun");
exception(exc_ts,"Invalid TSS Exception");
exception(exc_np,"Segment Not Present");
exception(exc_ss,"Stack Full Exception");
exception(exc_gp,"General Protection Exception");
exception(exc_pf,"Page-Fault Exception");
// no 15?
exception(exc_mf,"x87 FPU Floating_Point Error");
exception(exc_ac,"Alignment Check Exception");
exception(exc_mc,"Machine-Check Exception");
exception(exc_xf,"SIMD Floating-Point Exception");

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

        if(i==SYS_CALL){

          idt[i].dpl= 3; // system call handler has its DPL set to 3

        }

        idt[i].dpl = 0; // hardware interrupt handlers and exception handlers should have DPL=0

        idt[i].reserved0 = 0;
        idt[i].reserved1 = 1;
        idt[i].reserved2 = 1;
        idt[i].reserved3 = 1;  // load a descriptor with reserved bit
        idt[i].reserved4 = 0;

        idt[i].size = 1;  // 1 for 32 bit and 0 for 16 bit, set this file to 1 always

        if(i>=32){

           idt[i].reserved3 = 0; 
           //SET_IDT_ENTRY(idt[i], default_interrupt);
        }

        idt[i].seg_selector = KERNEL_CS;
    }



    // set up IDT entry 
    SET_IDT_ENTRY(idt[0],exc_de);
    SET_IDT_ENTRY(idt[1],exc_db);
    SET_IDT_ENTRY(idt[2],exc_nmi);
    SET_IDT_ENTRY(idt[3],exc_bp);
    SET_IDT_ENTRY(idt[4],exc_of);
    SET_IDT_ENTRY(idt[5],exc_br);
    SET_IDT_ENTRY(idt[6],exc_ud);
    SET_IDT_ENTRY(idt[7],exc_nm);
    SET_IDT_ENTRY(idt[8],exc_df);
    SET_IDT_ENTRY(idt[9],exc_cso);
    SET_IDT_ENTRY(idt[10],exc_ts);
    SET_IDT_ENTRY(idt[11],exc_np);
    SET_IDT_ENTRY(idt[12],exc_ss);
    SET_IDT_ENTRY(idt[13],exc_gp);
    SET_IDT_ENTRY(idt[14],exc_pf);
    SET_IDT_ENTRY(idt[16],exc_mf);
    SET_IDT_ENTRY(idt[17],exc_ac);
    SET_IDT_ENTRY(idt[18],exc_mc);
    SET_IDT_ENTRY(idt[19],exc_xf);


}