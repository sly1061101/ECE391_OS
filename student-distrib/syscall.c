#include "syscall.h"
#include "file_system.h"
#include "keyboard.h"
#include "paging.h"
#include "x86_desc.h"
#include "syscall_helper.h"
#include "process.h"
#include "rtc.h"

#define VAL_2 2
#define VAL_22 22
#define USER_STACK_VIRTUAL_PAGE_INDEX 32
#define VID_PAGE_START 0x8000000
#define VID_PAGE_END 0x8400000
#define VIDEO 0xB8000
// jump table for various system calls
uint32_t syscall_jump_table[NUM_SYSCALL] =   {   0,
                                        (uint32_t)syscall_halt, (uint32_t)syscall_execute, (uint32_t)syscall_read,
                                        (uint32_t)syscall_write, (uint32_t)syscall_open, (uint32_t)syscall_close,
                                        (uint32_t)syscall_getargs, (uint32_t)syscall_vidmap, (uint32_t)syscall_set_handler,
                                        (uint32_t)syscall_sigreturn
                                    };


// functions for stdin/out/rtc/file/dic for distinct tables 
fops_t stdin = {(read_t)terminal_read, NULL, NULL, NULL};
fops_t stdout = {NULL, (write_t)terminal_write, NULL, NULL};
fops_t rtc_ops = {(read_t)rtc_read, (write_t)rtc_write, (open_t)rtc_open, (close_t)rtc_close};
fops_t file_ops = {(read_t)file_read, (write_t)file_write, (open_t)file_open, (close_t)file_close};
fops_t dir_ops = {(read_t)directory_read, (write_t)directory_write, (open_t)directory_open, (close_t)directory_close}; 


// This function actually implements syscall_halt(). The reason to 
//  create another function is just to support return status greater
//  than 255, since the parameter of syscall_halt() is uint8 type and
//  we want status 256 when halted by exception.

/*
 *   halt_current_process
 *   DESCRIPTION: halt the current process (which is calling this function)
 *   INPUTS: status  -- return value of user program
 *   OUTPUTS: none 
 *   RETURN VALUE:  none on success
 *                 -1 on failure
 *   SIDE EFFECTS: none
 */
int32_t halt_current_process(uint32_t status) {
    int i;

    pcb_t *pcb = get_current_pcb();

    for(i=VAL_2;i<MAX_FD_SIZE;i++){
        if(pcb -> file_array[i].flag != 0){
             syscall_close(i);
        }
    }

    if(pcb->parent_pcb == NULL) {
        // If the first shell is halted, restart it automatically.
        (void) release_pid(pcb->pid);
        (void) syscall_execute((uint8_t*)"shell");
    }

    // The kernel space of a process in physical memory starts at 8MB - 8KB - 8KB * pid.
    uint32_t kernel_space_base_address = KERNEL_MEMORY_BOT - KERNEL_STACK_SIZE - KERNEL_STACK_SIZE * pcb->parent_pcb->pid;


    // Restore TSS for parent process.
    tss.ss0 = KERNEL_DS;
    tss.esp0 = kernel_space_base_address + KERNEL_STACK_SIZE - 1;

    // Restore page directory for parent process.
    load_page_directory(page_directory_program[pcb->parent_pid]);

    // Release the pid of current process.
    (void) release_pid(pcb->pid);

    // Restore esp and ebp for parent process, store status 
    //  to eax as execute() return value, then jump back to execute return.
    asm volatile("  movl %0, %%esp    \n\
                    movl %1, %%ebp    \n\
                    movl %2, %%eax    \n\
                    jmp syscall_execute_return "                             \
                :                                                            \
                :"r"(pcb->parent_esp),"r"(pcb->parent_ebp), "r"(status)      \
                :"memory");
    
    // Should never reach here.
    return -1;
}

/*
 *   syscall_halt
 *   DESCRIPTION: halt a process
 *   INPUTS: status  -- return status of program executed
 *   OUTPUTS: none
 *   RETURN VALUE: status
 *                 -1 on failure
 *   SIDE EFFECTS: none
 */

int32_t syscall_halt (uint8_t status) {
    return halt_current_process(status);
}

/*
 *   syscall_execute
 *   DESCRIPTION: execute a file
 *   INPUTS: command -- user command name
 *   OUTPUTS: none
 *   RETURN VALUE:  -1 on failure
 *                 -2 on number of process exceeding max                   
 *   SIDE EFFECTS: none
 */

int32_t syscall_execute (const uint8_t* command) {
    if(command == NULL)
        return -1;

    int i;

    // Parse the executable name.
    // TODO: Parse remaining arguments.
    uint8_t filename[FILE_NAME_MAX_LENGTH + 1];
    for(i = 0; i < strlen((int8_t*)command) + 1; ++i) {
        if(command[i] == ' ' || command[i] == '\0') {
            memcpy(filename, command, i);
            filename[i] = 0;
            break;
        }
        
        if(i >= FILE_NAME_MAX_LENGTH)
            return -1;
    }

    // Check if program exist in file system and has correct magic number.
    if(!check_executable(filename))
        return -1;
    
    // Request an available pid.
    uint32_t pid = request_pid();
    // If no more pid is available we cannot proceed.
    if(pid == -1) {
        printf("No more process is allowed!\n");
        return -2;
    }

    // The user space of a process in physical memory starts at 8MB + (pid * 4MB).
    uint32_t user_space_base_address = KERNEL_MEMORY_BOT + pid * USER_STACK_SIZE;
    // unused so far
    //uint32_t user_stack_size = 0x00400000;

    // Set up paging for user process.
    //  Video memory page and kernel memory page are the same with the initial setting.
    for(i = 0; i < NUM_PDT_SIZE; ++i)
        page_directory_program[pid][i] = page_directory_initial[i];

    // The 4MB page starting from 128MB should be mapped to correspoding physical page of a process's user space.
    page_directory_program[pid][USER_STACK_VIRTUAL_PAGE_INDEX].entry_page.present = 1;
    page_directory_program[pid][USER_STACK_VIRTUAL_PAGE_INDEX].entry_page.read_write = 1;
    page_directory_program[pid][USER_STACK_VIRTUAL_PAGE_INDEX].entry_page.user_supervisor = 1; // user privilege
    page_directory_program[pid][USER_STACK_VIRTUAL_PAGE_INDEX].entry_page.write_through = 0;
    page_directory_program[pid][USER_STACK_VIRTUAL_PAGE_INDEX].entry_page.cache_disabled = 0;
    page_directory_program[pid][USER_STACK_VIRTUAL_PAGE_INDEX].entry_page.accessed = 0;
    page_directory_program[pid][USER_STACK_VIRTUAL_PAGE_INDEX].entry_page.reserved = 0;
    page_directory_program[pid][USER_STACK_VIRTUAL_PAGE_INDEX].entry_page.page_size = 1; // 4MB page
    page_directory_program[pid][USER_STACK_VIRTUAL_PAGE_INDEX].entry_page.global_page = 0;
    page_directory_program[pid][USER_STACK_VIRTUAL_PAGE_INDEX].entry_page.available = 0;
    page_directory_program[pid][USER_STACK_VIRTUAL_PAGE_INDEX].entry_page.page_base_address = user_space_base_address >> VAL_22;

    // Load the above page directory.
    load_page_directory(page_directory_program[pid]);

    // Load executable into memory.
    uint32_t entry_address = load_executable(filename);
    if(entry_address == -1) {
        load_page_directory(page_directory_program[get_current_pcb()->pid]);
        return -1;
    }

    // The kernel space of a process in physical memory starts at 8MB - 8KB - 8KB * pid.
    uint32_t kernel_space_base_address = KERNEL_MEMORY_BOT - KERNEL_STACK_SIZE - KERNEL_STACK_SIZE * pid;

    // TODO: PCB stuffs need to be refined.
    
    // Set up PCB for user process.
    pcb_t *pcb = (pcb_t *)kernel_space_base_address;
    pcb->pid = pid;
        
    // Initialize file descriptor array.
    pcb->file_array[0].fops = &stdin;
    pcb->file_array[1].fops = &stdout;

    pcb->file_array[0].flag = 1;
    pcb->file_array[1].flag = 1;

    for(i = 2; i < MAX_FD_SIZE; i++)
        pcb -> file_array[i].flag = 0; 

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
    tss.esp0 = kernel_space_base_address + KERNEL_STACK_SIZE - 1;

    // PUSH IRET context and switch to user mode.
    switch_to_user(USER_DS, USER_STACK_BOTTOM_VIRTUAL, USER_CS, entry_address);

    asm volatile ("syscall_execute_return: leave; ret;");

    return -1;
}

/*
 *   syscall_read
 *   DESCRIPTION: read a file
 *   INPUTS: fd -- file descriptor 
             buf -- buffer of destination of copied file
             nbytes -- number of bytes
 *   OUTPUTS: none
 *   RETURN VALUE: number of bytes on success, -1 on failure
 *   SIDE EFFECTS: none
 */
int32_t syscall_read (int32_t fd, void* buf, int32_t nbytes) {

    sti();

    // error handling
    if(fd >= MAX_FD_SIZE || fd < 0 || buf == NULL)
    {
        return -1;
    }
    // get current pcb
    pcb_t * curr_pcb = get_current_pcb(); 

    // call read function
    int num_byte_read = curr_pcb->file_array[fd].fops->read_func(fd , buf , nbytes);

    // error condition when return byte is negative
    if(num_byte_read < 0)
    {
        return -1;
    }
    return num_byte_read;
}



/*
 *   syscall_write
 *   DESCRIPTION: write a file
 *   INPUTS: fd -- file descriptor 
             buf -- buffer of destination of copied file
             nbytes -- number of bytes
 *   OUTPUTS: none
 *   RETURN VALUE: number of bytes on success, -1 on failure
 *   SIDE EFFECTS: none
 */
int32_t syscall_write (int32_t fd, const void* buf, int32_t nbytes) {

    // handle error situation
   if(fd >= MAX_FD_SIZE || fd < 0 || buf == NULL)
   {
    return -1;
    }

    // get current pcd
    pcb_t * curr_pcb = get_current_pcb();


    // call write function   
    int num_byte_write = curr_pcb->file_array[fd].fops->write_func(fd , buf , nbytes);

    // error condition when return byte is negative
    if(num_byte_write < 0)
    {
        return -1;
    }
    return num_byte_write;

}


/*
 *   syscall_open
 *   DESCRIPTION: open a file
 *   INPUTS: filename  --  file to be opened
 *   OUTPUTS: none
 *   RETURN VALUE: file descriptor index
 *                 -1 on failure
 *   SIDE EFFECTS: none
 */
int32_t syscall_open(const uint8_t* filename) {

    int32_t i = 0;
    dentry_t fileopen;
    
    // error handling
    if (filename == NULL || read_dentry_by_name(filename, &fileopen) == -1){
        return -1;
    }

    //create current pcb
    pcb_t * curr_pcb = get_current_pcb();


    // find the next available entry in fd
    for (i = 0; i < MAX_FD_SIZE; i++)
    {
        if (curr_pcb->file_array[i].flag == 0){
            break;
        }
    }

    // when pcd arry is full
    if(i == MAX_FD_SIZE) return -1;

    // set current pcb in use
    curr_pcb->file_array[i].flag = 1;

    // determine file type
    switch(fileopen.file_type){

        case RTC_TYPE:
        curr_pcb -> file_array[i].fops = &rtc_ops;
        curr_pcb -> file_array[i].inode = fileopen.inode_idx;
        curr_pcb -> file_array[i].fops->open_func(filename);
        return i;

        case DIR_TYPE:
        curr_pcb -> file_array[i].fops = &dir_ops;
        curr_pcb -> file_array[i].inode = fileopen.inode_idx;
        curr_pcb -> file_array[i].fops->open_func(filename);
        return i;

        case FILE_TYPE:
        curr_pcb -> file_array[i].fops = &file_ops;
        curr_pcb -> file_array[i].inode = fileopen.inode_idx;
        curr_pcb -> file_array[i].fops->open_func(filename);
        return i;

        default:
        return -1;

    }
}

/*
 *   syscall_close
 *   DESCRIPTION: close a file
 *   INPUTS: file descriptor 
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success, -1 on failure
 *   SIDE EFFECTS: none
 */
int32_t syscall_close(int32_t fd) {

    // handle file descriptor size error condition
    if (fd < MIN_FD_SIZE || fd >= MAX_FD_SIZE) {
        return -1;
    }

    // get current pcb
    pcb_t *curr_pcb = get_current_pcb();

    // if currnet in not in use, can't close
    if (curr_pcb -> file_array[fd].flag == 0) {
        return -1;
    }

    // close the file
    curr_pcb -> file_array[fd].fops->close_func(fd);
    curr_pcb -> file_array[fd].flag = 0;
    curr_pcb -> file_array[fd].file_position = 0;
    curr_pcb -> file_array[fd].inode = 0;
    curr_pcb -> file_array[fd].fops = NULL;

    return 0;
}


// TODO
int32_t syscall_getargs (uint8_t* buf, int32_t nbytes) {
    return -1;
}

int32_t syscall_vidmap (uint8_t** screen_start) {
    if(screen_start < (uint8_t **)VID_PAGE_START || screen_start >= (uint8_t **)VID_PAGE_END)
    {
        return -1;
    }
    else
    {
        const uint32_t PD_entry_idx = 35;
        const uint32_t PT_entry_idx = 512;

        // Set up page table.
        page_table_program_vidmap[PT_entry_idx].present = 1;
        page_table_program_vidmap[PT_entry_idx].read_write = 1;
        page_table_program_vidmap[PT_entry_idx].user_supervisor = 1;
        page_table_program_vidmap[PT_entry_idx].write_through = 0;
        page_table_program_vidmap[PT_entry_idx].cache_disabled = 0;
        page_table_program_vidmap[PT_entry_idx].accessed = 0;
        page_table_program_vidmap[PT_entry_idx].dirty = 0;
        page_table_program_vidmap[PT_entry_idx].pt_attribute_index = 0;
        page_table_program_vidmap[PT_entry_idx].global_page = 0;
        page_table_program_vidmap[PT_entry_idx].available = 0;
        page_table_program_vidmap[PT_entry_idx].page_base_address = VIDEO >> 12;

        // Modify processs page directory.
        uint32_t pid = get_current_pcb()->pid;
        page_directory_program[pid][PD_entry_idx].entry_PT.present = 1;
        page_directory_program[pid][PD_entry_idx].entry_PT.read_write = 1;
        page_directory_program[pid][PD_entry_idx].entry_PT.user_supervisor = 1; // user privilege
        page_directory_program[pid][PD_entry_idx].entry_PT.write_through = 0;
        page_directory_program[pid][PD_entry_idx].entry_PT.cache_disabled = 0;
        page_directory_program[pid][PD_entry_idx].entry_PT.accessed = 0;
        page_directory_program[pid][PD_entry_idx].entry_PT.reserved = 0;
        page_directory_program[pid][PD_entry_idx].entry_PT.page_size = 0; // 4KB page table
        page_directory_program[pid][PD_entry_idx].entry_PT.global_page = 0;
        page_directory_program[pid][PD_entry_idx].entry_PT.available = 0;
        page_directory_program[pid][PD_entry_idx].entry_PT.pt_base_address = (uint32_t)page_table_program_vidmap >> 12;

        // TODO: This Page Directory Entry should be cleared when process is halted.

        // Calculate address.
        *screen_start = PD_entry_idx * 4 * 1024 * 1024 + PT_entry_idx * 4 * 1024;

        return  0;
    }
}

int32_t syscall_set_handler (int32_t signum, void* handler) {
    return -1;
}

int32_t syscall_sigreturn (void) {
    return -1;
}
