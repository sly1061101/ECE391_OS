# paging.S
# vim:ts=4 noexpandtab

#define ASM     1
#include "paging.h"


#define NOT_PRESENT_PAGE    0x00000002
#define FOUR_MB_PAGE            0x00400083
#define VIDEO_MEM_PAGE      0x000B8007
.text

.globl page_directory_initial
.globl page_table_initial
.global page_directory_program
.globl enable_paging
.global load_page_directory
.global paging_init
.global page_table_program_vidmap

# PDT, align to 4KB boundary
    .align  4096
page_directory_initial:
    .long page_table_initial + 0x03 # first 4MB uses the page table defined below
    .long FOUR_MB_PAGE # map second 4MB page to physical address space starting from 0x00400000

    .rept 1022
    .long NOT_PRESENT_PAGE # all other PTs are not present
    .endr

# PT, align to 4KB boundary
    .align  4096
page_table_initial:
    .rept 184 # first 184 pages (virtual address 0x00000000 to 0x000B7FFF) not present
    .long NOT_PRESENT_PAGE
    .endr

    .long VIDEO_MEM_PAGE # map video memory page (virtual address 0x000B8000 to 0x000B8FFF) to corresponding physical page (0x000B8000 to 0x000B8FFF)

    .rept 839 # remaining pages in this PT also not present
    .long NOT_PRESENT_PAGE
    .endr

# Page directory for user programs, will be set up in execute syscall.
    .align 4096
page_directory_program:
    .rept MAX_PROCESS_NUMBER * NUM_PDT_SIZE
    .long NOT_PRESENT_PAGE
    .endr

# Page table for user program video memory mapping.
    .align  4096
page_table_program_vidmap:
    .rept 1024
    .long NOT_PRESENT_PAGE
    .endr

# enable_paging
# DISCRIPTION: function to set registers and enable paging
# INPUT: NONE
# OUTPUT: NONE
# RETURN VALUE: NONE
# SIDE EFFECTS: NONE
enable_paging:
    pushl %ebp
    movl %esp, %ebp

    # Enable PSE
    movl %cr4, %ecx
    orl $0x00000010, %ecx
    movl %ecx, %cr4

    # Enable paging
    movl %cr0, %ecx
    orl $0x80000000, %ecx
    movl %ecx, %cr0

    leave
    ret

# Function to load a page directory into CR3 register.
# Input: page_directory - pointer to the page directory
load_page_directory:
    pushl %ebp
    movl %esp, %ebp

    # Load PDT
    movl %cr3, %ecx
    movl 8(%ebp), %edx
    andl $0x00000fff, %ecx
    andl $0xfffff000, %edx
    orl %edx, %ecx
    movl %ecx, %cr3

    leave
    ret

# Initialize paging stuffs.
# Load the initial page directory and turn on paging.
paging_init:
    pushl %ebp
    movl %esp, %ebp

    pushl $page_directory_initial
    call load_page_directory
    addl $4, %esp

    call enable_paging

    leave
    ret
