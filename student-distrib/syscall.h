#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#ifndef ASM

#include "types.h"

// #define MAX_FD_SIZE 8
// #define MIN_FD_SIZE 2
#define RTC_TYPE        0
#define DIR_TYPE        1
#define FILE_TYPE       2
#define MIN_FD_SIZE     2


// Use regular integer array to store function addresses directly.
//  Cannot use function pointer array because parameter lists are different.
extern uint32_t syscall_jump_table[11];

extern int32_t syscall_halt (uint8_t status);
extern int32_t syscall_execute (const uint8_t* command);
extern int32_t syscall_read (int32_t fd, void* buf, int32_t nbytes);
extern int32_t syscall_write (int32_t fd, const void* buf, int32_t nbytes);
extern int32_t syscall_open (const uint8_t* filename);
extern int32_t syscall_close (int32_t fd);
extern int32_t syscall_getargs (uint8_t* buf, int32_t nbytes);
extern int32_t syscall_vidmap (uint8_t** screen_start);
extern int32_t syscall_set_handler (int32_t signum, void* handler);
extern int32_t syscall_sigreturn (void);

#endif

#endif
