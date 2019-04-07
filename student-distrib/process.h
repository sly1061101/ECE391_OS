#ifndef _PROCESS_H_
#define _PROCESS_H_

#ifndef ASM

#include "types.h"

typedef struct pcb {
    uint32_t pid;
    uint32_t parent_pid;
    struct pcb *parent_pcb;
    uint32_t parent_ebp;
    uint32_t parent_esp;
} pcb_t;

// Current number of processes.
extern uint32_t process_count;

extern void process_init();

extern pcb_t* get_current_pcb();

#endif

#endif
