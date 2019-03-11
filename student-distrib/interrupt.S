# interrupt.S
# vim:ts=4 noexpandtab

#define ASM     1
#include "interrupt.h"
#include "idt.h"

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

common_interrupt:
    pushal
    # save interrupt number to eax
    movl 8*4(%esp), %eax
    movl $interrupt_handler, %ebx
    call *(%ebx, %eax, 4)
    popal
    addl $4, %esp
    iret

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
