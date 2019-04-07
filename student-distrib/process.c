#include "process.h"

// Current number of processes.
uint32_t process_count;

void process_init() {
    process_count = 0;
}

pcb_t* get_current_pcb() {
    uint32_t esp;

    asm volatile("movl %%esp, %0" \
                :"=r"(esp)   \
                :                \
                :"memory");

    pcb_t *pcb = esp & 0xffffe000;

    return pcb;
}
