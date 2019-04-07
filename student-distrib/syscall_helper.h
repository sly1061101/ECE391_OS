#ifndef _SYSCALL_HELPER_H_
#define _SYSCALL_HELPER_H_

#ifndef ASM

#include "types.h"

extern void switch_to_user(uint32_t ds, uint32_t esp, uint32_t cs, uint32_t eip);

#endif

#endif
