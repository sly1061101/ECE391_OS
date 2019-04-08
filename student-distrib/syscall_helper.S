# syscall_helper.S
# vim:ts=4 noexpandtab

#define ASM     1
#include "syscall_helper.h"

.text

.global switch_to_user

# push iret contents to stack and switch to user process
switch_to_user:
    cli

    pushl %ebp
    movl %esp, %ebp

    # ds
    pushl 8(%ebp)

    # esp
    pushl 12(%ebp)

    # process and push eflags
    pushfl
    popl %eax
    orl $0x200, %eax
    pushl %eax

    # cs
    pushl 16(%ebp)

    # eip
    pushl 20(%ebp)

    iret
