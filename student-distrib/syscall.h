#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#ifndef ASM

#include "types.h"

#define RTC_TYPE        0
#define DIR_TYPE        1
#define FILE_TYPE       2
#define MIN_FD_SIZE     2

#define NUM_SYSCALL 11

// Use regular integer array to store function addresses directly.
// Cannot use function pointer array because parameter lists are different.
extern uint32_t syscall_jump_table[NUM_SYSCALL];

// system halt and execute
extern int32_t syscall_halt (uint8_t status);
extern int32_t syscall_execute (const uint8_t* command);

// system open/close/read/write
extern int32_t syscall_read (int32_t fd, void* buf, int32_t nbytes);
extern int32_t syscall_write (int32_t fd, const void* buf, int32_t nbytes);
extern int32_t syscall_open (const uint8_t* filename);
extern int32_t syscall_close (int32_t fd);

// reads command line arguments into a user-level buffer
extern int32_t syscall_getargs (uint8_t* buf, int32_t nbytes);
// maps the video memory into user space
extern int32_t syscall_vidmap (uint8_t** screen_start);

// TODO for signal extra credit
extern int32_t syscall_set_handler (int32_t signum, void* handler);
extern int32_t syscall_sigreturn (void);

// helper function for syscall_halt
extern int32_t halt_current_process(uint32_t status);

#endif

#endif
