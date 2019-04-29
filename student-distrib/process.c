#include "process.h"
#include "terminal.h"
#include "paging.h"
#include "x86_desc.h"

// Current number of processes.
uint32_t process_count;
// Flags showing whether a process exists.
int8_t process_exist[MAX_PROCESS_NUMBER];
// Flag indicating whether process scheduling has been started.
int32_t scheduleing_started;

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
    scheduleing_started = 0;
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
    return (pcb_t*)(VAL_8 * VAL_1024 * VAL_1024 - VAL_8 * VAL_1024 - pid * VAL_8 * VAL_1024);
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

/*
 *   next_scheduled_process
 *   DESCRIPTION: for next scheduled process
 *   INPUTS: NONE
 *   OUTPUTS: i on success, -1 on failure
 *   SIDE EFFECTS: none
 */
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

/*
 *   switch_process
 *   DESCRIPTION: switch to other process
 *   INPUTS: NONE
 *   OUTPUTS: NONE
 *   SIDE EFFECTS: none
 */
void switch_process(uint32_t pid) {
    // get current pcb
    pcb_t *curr_pcb = get_current_pcb();

    // Save the current esp and ebp 
    asm volatile("movl %%esp, %0" \
                    :"=r"(curr_pcb->esp)   \
                    :                \
                    :"memory");

    asm volatile("movl %%ebp, %0" \
                    :"=r"(curr_pcb->ebp)   \
                    :                \
                    :"memory");

    // get next process to execute
    pcb_t *next_pcb = get_pcb(pid);

    // Backup the screen cursor position for current process and load it for next process.
    backup_screen_position(&screen_x_backstore[curr_pcb->terminal_id], &screen_y_backstore[curr_pcb->terminal_id]);
    load_screen_position(screen_x_backstore[next_pcb->terminal_id], screen_y_backstore[next_pcb->terminal_id]);
    // If next process is running on displayed terminal, update its cursor.
    if(next_pcb->terminal_id == get_display_terminal())
        update_cursor(screen_x_backstore[next_pcb->terminal_id], screen_y_backstore[next_pcb->terminal_id]);

    // Switch paging
    load_page_directory(page_directory_program[next_pcb->pid]);

    // The kernel space of a process in physical memory starts at 8MB - 8KB - 8KB * pid.
    uint32_t kernel_space_base_address = KERNEL_MEMORY_BOT - KERNEL_STACK_SIZE - KERNEL_STACK_SIZE * next_pcb->pid;

    // Modify TSS for context switch.
    tss.ss0 = KERNEL_DS;
    // Kernel stacks begins at highest address of kernel space and grows towards lower address.
    tss.esp0 = kernel_space_base_address + KERNEL_STACK_SIZE - 1;

    // change esp and ebp to next process's and switch to it
    asm volatile(
        "movl   %0, %%esp   ;"
        "movl   %1, %%ebp   ;"
        "LEAVE;"
        "RET;"
        : :"r"(next_pcb->esp), "r"(next_pcb->ebp) 
    );
}


/*
 *   start_scheduling
 *   DESCRIPTION: set flag to 1
 *   INPUTS: NONE
 *   OUTPUTS: NONE
 *   SIDE EFFECTS: none
 */
void start_scheduling() {
    scheduleing_started = 1;
}

/*
 *   is_scheduling_started
 *   DESCRIPTION: get the flag
 *   INPUTS: NONE
 *   OUTPUTS: NONE
 *   SIDE EFFECTS: none
 */
int32_t is_scheduling_started() {
    return scheduleing_started;
}

/*
 *   is_scheduling_started
 *   DESCRIPTION: get the process count
 *   INPUTS: NONE
 *   OUTPUTS: NONE
 *   SIDE EFFECTS: none
 */
 uint32_t get_process_count() {
     return process_count;
 }


