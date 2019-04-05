#include "syscall.h"
#include "file_system.h"
#include "keyboard.h"
#include "paging.h"

uint32_t syscall_jump_table[11] =   {   0,
                                        (uint32_t)syscall_halt, (uint32_t)syscall_execute, (uint32_t)syscall_read,
                                        (uint32_t)syscall_write, (uint32_t)syscall_open, (uint32_t)syscall_close,
                                        (uint32_t)syscall_getargs, (uint32_t)syscall_vidmap, (uint32_t)syscall_set_handler,
                                        (uint32_t)syscall_sigreturn
                                    };

int32_t syscall_halt (uint8_t status) {
    return -1;
}

uint32_t process_count = 0;

// We need to support 6 user processes at most.
pdt_entry_t page_directory_program[6][NUM_PDT_SIZE] __attribute__((aligned(4096)));

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
    
    // Initialize the program page diretory by initial page directory.
    for(i = 0; i < NUM_PDT_SIZE; ++i)
        page_directory_program[process_count][i] = page_directory_initial[i];

    // The 4MB page starting from 128MB would be mapped to physical memory at 8MB + (process number * 4MB)
    page_directory_program[process_count][32].entry_page.present = 1;
    page_directory_program[process_count][32].entry_page.read_write = 1;
    page_directory_program[process_count][32].entry_page.user_supervisor = 1; // user privilege
    page_directory_program[process_count][32].entry_page.write_through = 0;
    page_directory_program[process_count][32].entry_page.cache_disabled = 0;
    page_directory_program[process_count][32].entry_page.accessed = 0;
    page_directory_program[process_count][32].entry_page.reserved = 0;
    page_directory_program[process_count][32].entry_page.page_size = 1; // 4MB page
    page_directory_program[process_count][32].entry_page.global_page = 0;
    page_directory_program[process_count][32].entry_page.available = 0;
    page_directory_program[process_count][32].entry_page.page_base_address = 0x00800000 + process_count * 0x00400000;

    // Enable paging with the above page directory.
    enable_paging(page_directory_program[process_count]);

    // TODO: After this line, we must restore the page table on error.

    // Load executable into memory.
    uint32_t entry_address = load_executable(filename);
    if(entry_address == -1)
        return -1;

    process_count++;

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
