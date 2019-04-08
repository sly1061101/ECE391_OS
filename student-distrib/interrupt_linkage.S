# interrupt_linkage.S
# vim:ts=4 noexpandtab

#define ASM     1
#include "interrupt_linkage.h"
#include "idt.h"
#include "x86_desc.h"
#include "syscall.h"

.data

# variable to temporary save eax in system call linkage
eax_saved:
    .long 0

SYSCALL_NUM_MIN:
    .long 1

SYSCALL_NUM_MAX:
    .long 10

.text

.global common_interrupt
.global ir_linkage_0
.global ir_linkage_1
.global ir_linkage_2
.global ir_linkage_3
.global ir_linkage_4
.global ir_linkage_5
.global ir_linkage_6
.global ir_linkage_7
.global ir_linkage_8
.global ir_linkage_9
.global ir_linkage_10
.global ir_linkage_11
.global ir_linkage_12
.global ir_linkage_13
.global ir_linkage_14
.global ir_linkage_15
.global ir_linkage_16
.global ir_linkage_17
.global ir_linkage_18
.global ir_linkage_19
.global ir_linkage_33
.global ir_linkage_40
.global syscall_linkage
.global ir_linkage_default

# common_interrupt
# DISCRIPTION: handle all common interupt
# INPUT: NONE
# OUTPUT: NONE
# RETURN VALUE: NONE
# SIDE EFFECTS: NONE

common_interrupt:
    # save all general purpose registers (8 in total)
    pushal
    # load interrupt number and invoke corresponding handler from jump table
    movl 32(%esp), %eax
    movl $interrupt_handler, %ebx
    call *(%ebx, %eax, 4)
    # restore all general registers
    popal
    # remove interrupt number (pushed at ir_linkage) from stack
    addl $4, %esp
    # use iret to return
    iret

# each ir_linkage function simply pushes interrupt number into stack then jump to common handler
ir_linkage_0:
    pushl $0
    jmp common_interrupt

ir_linkage_1:
    pushl $1
    jmp common_interrupt

ir_linkage_2:
    pushl $2
    jmp common_interrupt

ir_linkage_3:
    pushl $3
    jmp common_interrupt

ir_linkage_4:
    pushl $4
    jmp common_interrupt

ir_linkage_5:
    pushl $5
    jmp common_interrupt

ir_linkage_6:
    pushl $6
    jmp common_interrupt

ir_linkage_7:
    pushl $7
    jmp common_interrupt

ir_linkage_8:
    pushl $8
    jmp common_interrupt

ir_linkage_9:
    pushl $9
    jmp common_interrupt

ir_linkage_10:
    pushl $10
    jmp common_interrupt

ir_linkage_11:
    pushl $11
    jmp common_interrupt

ir_linkage_12:
    pushl $12
    jmp common_interrupt

ir_linkage_13:
    pushl $13
    jmp common_interrupt

ir_linkage_14:
    pushl $14
    jmp common_interrupt

ir_linkage_15:
    pushl $15
    jmp common_interrupt

ir_linkage_16:
    pushl $16
    jmp common_interrupt

ir_linkage_17:
    pushl $17
    jmp common_interrupt

ir_linkage_18:
    pushl $18
    jmp common_interrupt

ir_linkage_19:
    pushl $19
    jmp common_interrupt

ir_linkage_33:
    pushl $33
    jmp common_interrupt

ir_linkage_40:
    pushl $40
    jmp common_interrupt

syscall_linkage:
    # save all registers
    pushal

    # check syscall number
    cmpl SYSCALL_NUM_MIN, %eax
    jl syscall_linkage_error
    cmpl SYSCALL_NUM_MAX, %eax
    jg syscall_linkage_error

    # parameters
    pushl %edx
    pushl %ecx
    pushl %ebx

    call *syscall_jump_table(, %eax, 4)

    addl $12, %esp

    # save eax value
    movl %eax, eax_saved

    # restore all registers
    popal
    movl eax_saved ,%eax
    iret

syscall_linkage_error:
    popal
    movl $-1, %eax
    iret
    
ir_linkage_default:
    pushl $NUM_VEC
    jmp common_interrupt
