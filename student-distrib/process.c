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

pcb_t* get_pcb(uint32_t pid) {
    // PCB is located at the top of kernel stack of each process. I.e. at 8MB - 8KB - pid * 8KB
    return (pcb_t*)(8 * 1024 * 1024 - 8 * 1024 - pid * 8 * 1024);
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

int32_t next_scheduled_process() {
    pcb_t *pcb = get_current_pcb();
    int32_t i;
    for(i = pcb->pid + 1; i < MAX_PROCESS_NUMBER; ++i) {
        if(process_exist[i] && get_pcb(i)->active)
            return i;
    }
    for(i = 0; i < pcb->pid; ++i) {
        if(process_exist[i] && get_pcb(i)->active)
            return i;
    }
    return -1;
}
