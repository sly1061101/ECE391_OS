#ifndef _PROCESS_H_
#define _PROCESS_H_

#define MAX_PROCESS_NUMBER 6

#ifndef ASM

#include "types.h"

#define MAX_FD_SIZE 8
#define MIN_FD_SIZE 2
#define HIGH_BIT_MASK 0xffffe000
#define MAX_ARG_SIZE 128

#define KERNEL_STACK_SIZE 0x2000
#define USER_STACK_SIZE 0x00400000
#define KERNEL_MEMORY_BOT 0x800000
#define USER_STACK_BOTTOM_VIRTUAL 0x83fffff

#define VAL_8 8
#define VAL_1024 1024

typedef int32_t (*read_t)(int32_t fd, void* buf, int32_t nbytes);
typedef int32_t (*write_t)(int32_t fd, const void* buf, int32_t nbytes);
typedef int32_t (*open_t)(const uint8_t* filename);
typedef int32_t (*close_t)(int32_t fd);

// fops struct
typedef struct fops {
    read_t read_func;
    write_t write_func;
    open_t open_func;
    close_t close_func;
} fops_t;

// file description struct
typedef struct file_desc {
    fops_t* fops;
    int32_t inode;
    int32_t file_position;
    int32_t flag;
} file_desc_t;

/*
    This struct saves information for each process.
        pid - the unique id for the process
        parent_pid - the pid for parent process, -1 if parent does not exist
        file_array - information for opened files for the process
        args_array - executing command for the process
        terminal_id - the id of the terminal this process running on
        active - whether this process is active for scheduling
        esp - the latest esp when the process became inactive (e.g. switch out by the scheduler)
        ebp - the latest esp when the process became inactive (e.g. switch out by the scheduler)
*/
typedef struct pcb {
    uint32_t pid;
    int32_t  parent_pid;
    file_desc_t file_array[MAX_FD_SIZE];
    int8_t  args_array[MAX_ARG_SIZE];
    int32_t terminal_id;
    int32_t active;
    uint32_t esp;
    uint32_t ebp;
} pcb_t;

// Flags showing whether a process exists.
extern int8_t process_exist[MAX_PROCESS_NUMBER];

// Initialize the process-related data structures
extern void process_init();

// helper function to get which kernel stack we are currently
extern pcb_t* get_current_pcb();

// get the pcb based on pid, undefined behavior if process not exist
extern pcb_t* get_pcb(uint32_t pid);

// Request an available pid. Return pid on success, or -1 if has reached max process count.
extern int32_t request_pid();

// Release the given pid, should be called when process is halted.
extern int32_t release_pid(uint32_t pid);
// get the next scheduled process i on success 0 on fail
extern int32_t next_scheduled_process();
// switch the process
extern void switch_process(uint32_t pid);
// set the schduling flag to 1
extern void start_scheduling();
//get the flag
extern int32_t is_scheduling_started();
//get the process count
extern uint32_t get_process_count();

#endif

#endif

