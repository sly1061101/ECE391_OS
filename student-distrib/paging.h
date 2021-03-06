#ifndef _PAGING_H
#define _PAGING_H

#include "types.h"
#include "process.h"
#include "terminal.h"

#define NUM_PDT_SIZE 1024
#define NUM_PT_SIZE 1024

#ifndef ASM

// struct for page directory table entry (4KB page table)
typedef struct __attribute__((packed)) pdt_entry_PT {
    uint32_t present:1;
    uint32_t read_write:1;
    uint32_t user_supervisor:1;
    uint32_t write_through:1;
    uint32_t cache_disabled:1;
    uint32_t accessed:1;
    uint32_t reserved:1;
    uint32_t page_size:1;
    uint32_t global_page:1;
    uint32_t available:3;
    uint32_t pt_base_address:20;
} pdt_entry_PT_t;

// struct for page directory table entry (4MB page)
typedef struct __attribute__((packed)) pdt_entry_page {
    uint32_t present:1;
    uint32_t read_write:1;
    uint32_t user_supervisor:1;
    uint32_t write_through:1;
    uint32_t cache_disabled:1;
    uint32_t accessed:1;
    uint32_t dirty:1;
    uint32_t page_size:1;
    uint32_t global_page:1;
    uint32_t available:3;
    uint32_t page_table_attribute_index:1;
    uint32_t reserved:9;
    uint32_t page_base_address:10;
} pdt_entry_page_t;

// struct for general page directory table entry
typedef union pdt_entry {
    pdt_entry_PT_t entry_PT;
    pdt_entry_page_t entry_page;
} pdt_entry_t;

// struct for page table entry
typedef struct __attribute__((packed)) pt_entry {
    uint32_t present:1;
    uint32_t read_write:1;
    uint32_t user_supervisor:1;
    uint32_t write_through:1;
    uint32_t cache_disabled:1;
    uint32_t accessed:1;
    uint32_t dirty:1;
    uint32_t pt_attribute_index:1;
    uint32_t global_page:1;
    uint32_t available:3;
    uint32_t page_base_address:20;
} pt_entry_t;

// PDT for entire memory space and PT for first 4MB.
extern pdt_entry_t page_directory_initial[NUM_PDT_SIZE];
extern pt_entry_t page_table_initial[NUM_PT_SIZE];

// Page directories for user programs, will be set up in execute syscall.
extern pdt_entry_t page_directory_program[MAX_PROCESS_NUMBER][NUM_PDT_SIZE];
// Page tables for terminals to map video memory.
extern pt_entry_t page_table_terminal_video_memory[TERMINAL_NUM][NUM_PT_SIZE];

// Page table for user program to map video memory into user-space, i.e. for the use of syscall_vidmap().
extern pt_entry_t page_table_program_vidmap[TERMINAL_NUM][NUM_PT_SIZE];

// Load a page directory into CR3 register.
extern void load_page_directory(pdt_entry_t page_directory_initial[NUM_PDT_SIZE]);

// Set up registers and turn on paging.
extern void enable_paging();

// Initialize the paging stuffs.
extern void paging_init();

#endif

#endif
