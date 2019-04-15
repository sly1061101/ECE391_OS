#ifndef _PROCESS_H_
#define _PROCESS_H_

#define MAX_PROCESS_NUMBER 6

#ifndef ASM

#include "types.h"

#define MAX_FD_SIZE 8
#define MIN_FD_SIZE 2
#define HIGH_BIT_MASK 0xffffe000
#define MAX_ARG_SIZE 128

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

// modified
typedef struct pcb {
    uint32_t pid;
    uint32_t parent_pid;
    struct pcb *parent_pcb;
    uint32_t parent_ebp;
    uint32_t parent_esp;
    file_desc_t file_array[MAX_FD_SIZE];
    int8_t  args_array[MAX_ARG_SIZE];
    char available[MAX_FD_SIZE]; // when is it used?
} pcb_t;

// Current number of processes.
extern uint32_t process_count;

// Flags showing whether a process exists.
extern int8_t process_exist[MAX_PROCESS_NUMBER];

// Initialize the process-related data structures
extern void process_init();

// helper function to get which kernel stack we are currently
extern pcb_t* get_current_pcb();

// Request an available pid. Return pid on success, or -1 if has reached max process count.
extern int32_t request_pid();

// Release the given pid, should be called when process is halted.
extern int32_t release_pid(uint32_t pid);

#endif

#endif
