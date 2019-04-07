#include "process.h"

// Current number of processes.
uint32_t process_count;
// Flags showing whether a process exists.
int8_t process_exist[MAX_PROCESS_NUMBER];

void process_init() {
    int i;
    for(i = 0; i < MAX_PROCESS_NUMBER; ++i)
        process_exist[i] = 0;
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

int32_t request_pid() {
    if(process_count >= MAX_PROCESS_NUMBER)
        return -1;
    
    int i;
    for(i = 0; i < MAX_PROCESS_NUMBER; ++i) {
        if(!process_exist[i]) {
            process_exist[i] = 1;
            process_count++;
            return i;
        }
    }

    // Should never reach here.
    return -1;
}

int32_t release_pid(uint32_t pid) {
    if(pid >= MAX_PROCESS_NUMBER)
        return -1;

    // TODO: We may also need to clean up the PCB stuffs here.
    process_exist[pid] = 0;
    process_count--;

    return 0;
}