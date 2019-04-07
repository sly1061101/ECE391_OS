#ifndef _PROCESS_H_
#define _PROCESS_H_

#ifndef ASM

#include "types.h"

#define MAX_PROCESS_NUMBER 6

typedef struct pcb {
    uint32_t pid;
    uint32_t parent_pid;
    struct pcb *parent_pcb;
    uint32_t parent_ebp;
    uint32_t parent_esp;
} pcb_t;

// Current number of processes.
extern uint32_t process_count;
// Flags showing whether a process exists.
extern int8_t process_exist[MAX_PROCESS_NUMBER];

extern void process_init();

extern pcb_t* get_current_pcb();

// Request an available pid. Return pid on success, or -1 if has reached max process count.
extern int32_t request_pid();

// Release the given pid, should be called when process is halted.
extern int32_t release_pid(uint32_t pid);

#endif

#endif
