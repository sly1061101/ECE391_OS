#include "process.h"


// Current number of processes.
uint32_t process_count;
// Flags showing whether a process exists.
int8_t process_exist[MAX_PROCESS_NUMBER];

/*
 *   process_init
 *   DESCRIPTION: Initialize process-related data structre
 *   INPUTS: none
 *   OUTPUTS: none
 *   SIDE EFFECTS: none
 */
void process_init() {
    int i;
    for(i = 0; i < MAX_PROCESS_NUMBER; ++i)
        process_exist[i] = 0;
    process_count = 0;
}

/*
 *   get_current_pcb
 *   DESCRIPTION: get current pcb based on which kernel stack we are on 
 *   INPUTS: none
 *   OUTPUTS: pcb pointer
 *   SIDE EFFECTS: none
 */
pcb_t* get_current_pcb() {
    uint32_t esp;

    asm volatile("movl %%esp, %0" \
                :"=r"(esp)   \
                :                \
                :"memory");

    pcb_t* pcb = (pcb_t*) (esp & HIGH_BIT_MASK);

    return pcb;
}

/*
 *   request_pid
 *   DESCRIPTION: return the next unused pid
 *   INPUTS: none
 *   OUTPUTS: pid number
 *   SIDE EFFECTS: none
 */
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
/*
 *   release_pid
 *   DESCRIPTION: release the given pid
 *   INPUTS: pid number
 *   OUTPUTS: 0 on success, -1 on failure
 *   SIDE EFFECTS: none
 */
int32_t release_pid(uint32_t pid) {
    if(pid >= MAX_PROCESS_NUMBER)
        return -1;

    // TODO: We may also need to clean up the PCB stuffs here.
    process_exist[pid] = 0;
    process_count--;

    return 0;
}
