# paging.S
# vim:ts=4 noexpandtab

#define ASM     1
#include "paging.h"

.text

.globl page_directory_table
.globl page_table
.globl enable_paging

# PDT, align to 4KB boundary
    .align  4096
page_directory_table:
    .long page_table + 0x03 # first 4MB uses the page table defined below
    .long 0x00400083 # map second 4MB page to physical address space starting from 0x00400000

    .rept 1022
    .long 0x00000002 # all other PTs are not present
    .endr

# PT, align to 4KB boundary
    .align  4096
page_table:
    .rept 184 # first 184 pages (virtual address 0x00000000 to 0x000B7FFF) not present
    .long 0x00000002
    .endr

    .long 0x000B8007 # map video memory page (virtual address 0x000B8000 to 0x000B8FFF) to corresponding physical page (0x000B8000 to 0x000B8FFF)

    .rept 839 # remaining pages in this PT also not present
    .long 0x00000002
    .endr

# function to set registers and enable paging
enable_paging:
    pushl %ebp
    movl %esp, %ebp

    # Load PDT
    movl %cr3, %ecx
    leal page_directory_table, %edx
    andl $0x00000fff, %ecx
    andl $0xfffff000, %edx
    orl %edx, %ecx
    movl %ecx, %cr3

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