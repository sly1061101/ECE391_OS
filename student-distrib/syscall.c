#include "syscall.h"
#include "file_system.h"
#include "keyboard.h"

uint32_t syscall_jump_table[11] =   {   0,
                                        (uint32_t)syscall_halt, (uint32_t)syscall_execute, (uint32_t)syscall_read,
                                        (uint32_t)syscall_write, (uint32_t)syscall_open, (uint32_t)syscall_close,
                                        (uint32_t)syscall_getargs, (uint32_t)syscall_vidmap, (uint32_t)syscall_set_handler,
                                        (uint32_t)syscall_sigreturn
                                    };

int32_t syscall_halt (uint8_t status) {
    return -1;
}

int32_t syscall_execute (const uint8_t* command) {
    if(command == NULL)
        return -1;
    int i;

    // Parse the executable name.
    // TODO: Parse remaining arguments.
    uint8_t filename[FILE_NAME_MAX_LENGTH + 1];
    for(i = 0; i < strlen(command) + 1; ++i) {
        if(command[i] == ' ' || command[i] == '\0') {
            memcpy(filename, command, i);
            filename[i] = 0;
            break;
        }
        
        if(i >= FILE_NAME_MAX_LENGTH)
            return -1;
    }

    if(!check_executable(filename))
        return -1;
    
    

    return -1;
}

int32_t syscall_read (int32_t fd, void* buf, int32_t nbytes) {
    return -1;
}

int32_t syscall_write (int32_t fd, const void* buf, int32_t nbytes) {
    return -1;
}

int32_t syscall_open (const uint8_t* filename) {
    return -1;
}

int32_t syscall_close (int32_t fd) {
    return -1;
}

int32_t syscall_getargs (uint8_t* buf, int32_t nbytes) {
    return -1;
}

int32_t syscall_vidmap (uint8_t** screen_start) {
    return -1;
}

int32_t syscall_set_handler (int32_t signum, void* handler) {
    return -1;
}

int32_t syscall_sigreturn (void) {
    return -1;
}
