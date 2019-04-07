#include "syscall.h"
#include "file_system.h"
#include "keyboard.h"
#include "paging.h"
#include "x86_desc.h"
#include "syscall_helper.h"
#include "process.h"

uint32_t syscall_jump_table[11] =   {   0,
                                        (uint32_t)syscall_halt, (uint32_t)syscall_execute, (uint32_t)syscall_read,
                                        (uint32_t)syscall_write, (uint32_t)syscall_open, (uint32_t)syscall_close,
                                        (uint32_t)syscall_getargs, (uint32_t)syscall_vidmap, (uint32_t)syscall_set_handler,
                                        (uint32_t)syscall_sigreturn
                                    };

// We need to support 6 user processes at most.
pdt_entry_t page_directory_program[MAX_PROCESS_NUMBER][NUM_PDT_SIZE] __attribute__((aligned(4096)));

int32_t syscall_halt (uint8_t status) {
    pcb_t *pcb = get_current_pcb();

    if(pcb->parent_pcb == NULL) {
        printf("The first shell should not be halted.\n");
        while(1);
    }

    // The kernel space of a process in physical memory starts at 8MB - 8KB - 8KB * pid.
    uint32_t kernel_space_base_address = 0x800000 - 0x2000 - 0x2000 * pcb->parent_pcb->pid;
    // Kernel stack size is 8KB.
    uint32_t kernel_stack_size = 0x2000;

    // Restore TSS for parent process.
    tss.ss0 = KERNEL_DS;
    tss.esp0 = kernel_space_base_address + kernel_stack_size - 1;

    // Restore page directory for parent process.
    enable_paging(page_directory_program[pcb->parent_pid]);

    // Release the pid of current process.
    (void) release_pid(pcb->pid);

    // Restore esp and ebp for parent process.
    asm volatile("  movl %0, %%esp    \n\
                    movl %1, %%ebp"
                :                                               \
                :"r"(pcb->parent_esp),"r"(pcb->parent_ebp)      \
                :"memory");

    // Store the status to eax for syscall_execute() to use return value.
    asm volatile("movl %0, %%eax"        \
                :                        \
                :"r"((uint32_t)status)   \
                :"memory");

    // Go back to execute return at parent process.
    asm volatile ("jmp syscall_execute_return;");

    // Should never reach here.
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
    
    // Request an available pid.
    uint32_t pid = request_pid();
    // If no more pid is available we cannot proceed.
    if(pid == -1)
        return -1;

    // The user space of a process in physical memory starts at 8MB + (pid * 4MB).
    uint32_t user_space_base_address = 0x00800000 + pid * 0x00400000;
    uint32_t user_stack_size = 0x00400000;

    // Video memory page and kernel memory page are the same with the initial setting.
    for(i = 0; i < NUM_PDT_SIZE; ++i)
        page_directory_program[pid][i] = page_directory_initial[i];

    // The 4MB page starting from 128MB should be mapped to correspoding physical page of a process's user space.
    page_directory_program[pid][32].entry_page.present = 1;
    page_directory_program[pid][32].entry_page.read_write = 1;
    page_directory_program[pid][32].entry_page.user_supervisor = 1; // user privilege
    page_directory_program[pid][32].entry_page.write_through = 0;
    page_directory_program[pid][32].entry_page.cache_disabled = 0;
    page_directory_program[pid][32].entry_page.accessed = 0;
    page_directory_program[pid][32].entry_page.reserved = 0;
    page_directory_program[pid][32].entry_page.page_size = 1; // 4MB page
    page_directory_program[pid][32].entry_page.global_page = 0;
    page_directory_program[pid][32].entry_page.available = 0;
    page_directory_program[pid][32].entry_page.page_base_address = user_space_base_address >> 22;

    // Enable paging with the above page directory.
    enable_paging(page_directory_program[pid]);

    // TODO: After this line, we must restore the page directory on error.

    // Load executable into memory.
    uint32_t entry_address = load_executable(filename);
    if(entry_address == -1)
        return -1;

    // The kernel space of a process in physical memory starts at 8MB - 8KB - 8KB * pid.
    uint32_t kernel_space_base_address = 0x800000 - 0x2000 - 0x2000 * pid;
    // Kernel stack size is 8KB.
    uint32_t kernel_stack_size = 0x2000;

    // TODO: PCB stuffs need to be refined.
    pcb_t *pcb = (pcb_t *)kernel_space_base_address;
    pcb->pid = pid;

    if(pid == 0) {
        // First process does not have parent.
        pcb->parent_pid = -1;
        pcb->parent_pcb = NULL;
    }
    else {
        // Current process is the parent of the program to be executed.
        pcb->parent_pcb = get_current_pcb();
        pcb->parent_pid = pcb->parent_pcb->pid;

        // Save the current esp and ebp data so that they can be restored at syscall_halt().
        uint32_t esp;
        uint32_t ebp;

        asm volatile("movl %%esp, %0" \
                    :"=r"(esp)   \
                    :                \
                    :"memory");

        asm volatile("movl %%ebp, %0" \
                    :"=r"(ebp)   \
                    :                \
                    :"memory");

        pcb->parent_esp = esp;
        pcb->parent_ebp = ebp;
    }

    // Modify TSS for context switch.
    tss.ss0 = KERNEL_DS;
    // Kernel stacks begins at highest address of kernel space and grows towards lower address.
    tss.esp0 = kernel_space_base_address + kernel_stack_size - 1;

    // PUSH IRET context and switch to user mode.
    //  0x83fffff is the highest virtual address of user stack.
    switch_to_user(USER_DS, 0x83fffff, USER_CS, entry_address);

    asm volatile ("syscall_execute_return: leave; ret;");

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
