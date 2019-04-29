#include "syscall.h"
#include "file_system.h"
#include "keyboard.h"
#include "paging.h"
#include "x86_desc.h"
#include "syscall_helper.h"
#include "process.h"
#include "rtc.h"
#include "terminal.h"

#define VAL_2 2
#define VAL_22 22
#define USER_STACK_VIRTUAL_PAGE_INDEX 32
#define VID_PAGE_START 0x8000000
#define VID_PAGE_END 0x8400000
#define VIDEO 0xB8000
#define VAL_1024 1024
#define VAL_4 4
#define VAL_12 12
#define PD_ENTRY_IDX 35
#define PT_ENTRY_IDX 512
// jump table for various system calls
uint32_t syscall_jump_table[NUM_SYSCALL] =   {   0,
                                        (uint32_t)syscall_halt, (uint32_t)syscall_execute, (uint32_t)syscall_read,
                                        (uint32_t)syscall_write, (uint32_t)syscall_open, (uint32_t)syscall_close,
                                        (uint32_t)syscall_getargs, (uint32_t)syscall_vidmap, (uint32_t)syscall_set_handler,
                                        (uint32_t)syscall_sigreturn
                                    };


// functions for stdin/out/rtc/file/dic for distinct tables 
fops_t stdin = {(read_t)terminal_read, (write_t)terminal_write, (open_t)terminal_open, (close_t)terminal_close};
fops_t stdout = {(read_t)terminal_read, (write_t)terminal_write, (open_t)terminal_open, (close_t)terminal_close};
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

    if(pcb->parent_pid == -1) {
        // If the first shell on any terminal is halted, restart it automatically.
        (void) release_pid(pcb->pid);
        // Mark the terminal to be inactive so that syscall_execute() could find 
        //  it and correctly handle the situation.
        set_terminal_state(get_current_pcb()->terminal_id, TERMINAL_INACTIVE);
        (void) syscall_execute((uint8_t*)"shell");
    }

    pcb_t *parent_pcb = get_pcb(pcb->parent_pid);

    parent_pcb->active = 1;

    // The kernel space of a process in physical memory starts at 8MB - 8KB - 8KB * pid.
    uint32_t kernel_space_base_address = KERNEL_MEMORY_BOT - KERNEL_STACK_SIZE - KERNEL_STACK_SIZE * parent_pcb->pid;


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
                :"r"(parent_pcb->esp),"r"(parent_pcb->ebp), "r"(status)      \
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

#define PT_IDX_VIDEO_MEM 184
#define PT_IDX_ALWAY_TO_PHYSICAL_VIDEO_MEM 185
#define PD_IDX_FIRST_4MB 0

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
    int j;
    // Parse the executable name.
    // TODO: Parse remaining arguments.
    uint8_t filename[FILE_NAME_MAX_LENGTH + 1];
    uint8_t args[MAX_ARG_SIZE];
    for(i = 0; i < strlen((int8_t*)command) + 1; ++i) {
        if(command[i] == ' ' || command[i] == '\0') {
            memcpy(filename, command, i);
            filename[i] = 0;
            break;
        }
        
        if(i >= FILE_NAME_MAX_LENGTH)
            return -1;
    }

        j = i;
    while(command[j] == ' ') {
        // strip spaces
        j++;
    }
    i = 0;
    while(command[j] != '\0' && command[j] != '\n' && command[j] != '\r' && j < MAX_ARG_SIZE) {
        // get args
        args[i] = command[j];
        i++;
        j++;
    }
   
    // assign null to the last element
    args[i] = '\0';


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

    pcb_t *pcb = get_pcb(pid);

    // Decide which terminal this process will be running on. If there are inactive terminals, 
    //  that means the system is sill initializing, so the process should run on next inactive
    //  terminal. Otherwise, the process should just follow its parent process.
    int32_t next_inactive_terminal = get_next_inactive_terminal();
    if(next_inactive_terminal != -1) {
        pcb->terminal_id = next_inactive_terminal;
        set_terminal_state(pcb->terminal_id, TERMINAL_ACTIVE);
        // Set up page table for processes running on this terminal.
        page_table_terminal_video_memory[pcb->terminal_id][PT_IDX_VIDEO_MEM].present = 1;
        page_table_terminal_video_memory[pcb->terminal_id][PT_IDX_VIDEO_MEM].read_write = 1;
        page_table_terminal_video_memory[pcb->terminal_id][PT_IDX_VIDEO_MEM].user_supervisor = 1;
        page_table_terminal_video_memory[pcb->terminal_id][PT_IDX_VIDEO_MEM].write_through = 0;
        page_table_terminal_video_memory[pcb->terminal_id][PT_IDX_VIDEO_MEM].cache_disabled = 0;
        page_table_terminal_video_memory[pcb->terminal_id][PT_IDX_VIDEO_MEM].accessed = 0;
        page_table_terminal_video_memory[pcb->terminal_id][PT_IDX_VIDEO_MEM].dirty = 0;
        page_table_terminal_video_memory[pcb->terminal_id][PT_IDX_VIDEO_MEM].pt_attribute_index = 0;
        page_table_terminal_video_memory[pcb->terminal_id][PT_IDX_VIDEO_MEM].global_page = 0;
        page_table_terminal_video_memory[pcb->terminal_id][PT_IDX_VIDEO_MEM].available = 0;
        // If this terminal is currently displayed, map video memory addresses to physical video memory.
        //  Otherwise map to video memory backstorage
        if(pcb->terminal_id == get_display_terminal())
            page_table_terminal_video_memory[pcb->terminal_id][PT_IDX_VIDEO_MEM].page_base_address = VIDEO >> VAL_12;
        else
            page_table_terminal_video_memory[pcb->terminal_id][PT_IDX_VIDEO_MEM].page_base_address = (uint32_t)(video_mem_backstore[pcb->terminal_id]) >> VAL_12;

        // Always map the next 4KB page to physical video memory so that each process still has access.
        page_table_terminal_video_memory[pcb->terminal_id][PT_IDX_ALWAY_TO_PHYSICAL_VIDEO_MEM].present = 1;
        page_table_terminal_video_memory[pcb->terminal_id][PT_IDX_ALWAY_TO_PHYSICAL_VIDEO_MEM].read_write = 1;
        page_table_terminal_video_memory[pcb->terminal_id][PT_IDX_ALWAY_TO_PHYSICAL_VIDEO_MEM].user_supervisor = 1;
        page_table_terminal_video_memory[pcb->terminal_id][PT_IDX_ALWAY_TO_PHYSICAL_VIDEO_MEM].write_through = 0;
        page_table_terminal_video_memory[pcb->terminal_id][PT_IDX_ALWAY_TO_PHYSICAL_VIDEO_MEM].cache_disabled = 0;
        page_table_terminal_video_memory[pcb->terminal_id][PT_IDX_ALWAY_TO_PHYSICAL_VIDEO_MEM].accessed = 0;
        page_table_terminal_video_memory[pcb->terminal_id][PT_IDX_ALWAY_TO_PHYSICAL_VIDEO_MEM].dirty = 0;
        page_table_terminal_video_memory[pcb->terminal_id][PT_IDX_ALWAY_TO_PHYSICAL_VIDEO_MEM].pt_attribute_index = 0;
        page_table_terminal_video_memory[pcb->terminal_id][PT_IDX_ALWAY_TO_PHYSICAL_VIDEO_MEM].global_page = 0;
        page_table_terminal_video_memory[pcb->terminal_id][PT_IDX_ALWAY_TO_PHYSICAL_VIDEO_MEM].available = 0;
        page_table_terminal_video_memory[pcb->terminal_id][PT_IDX_ALWAY_TO_PHYSICAL_VIDEO_MEM].page_base_address = VIDEO >> VAL_12;
    }
    else {
        pcb->terminal_id = get_current_pcb()->terminal_id;
    }

    // The user space of a process in physical memory starts at 8MB + (pid * 4MB).
    uint32_t user_space_base_address = KERNEL_MEMORY_BOT + pid * USER_STACK_SIZE;
    // unused so far
    //uint32_t user_stack_size = 0x00400000;

    // Set up page directory for user process based on the initial page directory.
    //  Kernel memory page are the same with the initial setting.
    for(i = 0; i < NUM_PDT_SIZE; ++i)
        page_directory_program[pid][i] = page_directory_initial[i];

    // Page table for video memory should be changed to corresponding terminal's.
    page_directory_program[pid][PD_IDX_FIRST_4MB].entry_PT.pt_base_address = (uint32_t)page_table_terminal_video_memory[pcb->terminal_id] >> VAL_12;

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

    // Set up PCB for user process.
    pcb->pid = pid;

    memcpy(pcb->args_array,args,MAX_ARG_SIZE);
        
    // Initialize file descriptor array.
    pcb->file_array[0].fops = &stdin;
    pcb->file_array[1].fops = &stdout;

    pcb->file_array[0].flag = 1;
    pcb->file_array[1].flag = 1;

    for(i = 2; i < MAX_FD_SIZE; i++)
        pcb -> file_array[i].flag = 0; 

    if(next_inactive_terminal != -1) {
        // We consider first process of each terminal not having parent.
        pcb->parent_pid = -1;
    }
    else {
        pcb_t *parent_pcb = get_current_pcb();
        // Current process is the parent of the program to be executed.
        pcb->parent_pid = parent_pcb->pid;
        // Mark its parent to be inactive so that scheduler will ignore it.
        parent_pcb->active = 0;

        // Save the current esp and ebp data so that they can be restored at syscall_halt().
        uint32_t esp;
        uint32_t ebp;

        asm volatile("movl %%esp, %0" \
                    :"=r"(parent_pcb->esp)   \
                    :                \
                    :"memory");

        asm volatile("movl %%ebp, %0" \
                    :"=r"(parent_pcb->ebp)   \
                    :                \
                    :"memory");
    }

    // Mark the process to be executed to active.
    pcb->active = 1;

    // The kernel space of a process in physical memory starts at 8MB - 8KB - 8KB * pid.
    uint32_t kernel_space_base_address = KERNEL_MEMORY_BOT - KERNEL_STACK_SIZE - KERNEL_STACK_SIZE * pid;

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

    if(curr_pcb->file_array[fd].flag == 0)
        return -1;

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

    if(curr_pcb->file_array[fd].flag == 0)
        return -1;

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

/*
 *   syscall_getargs
 *   DESCRIPTION: reads command line arguments into
 *                a user-level buffer
 *   INPUTS: buf -- user-level buffer
 *           nbytes -- maximum number of bytes 
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success, -1 on failure
 *   SIDE EFFECTS: none
 */
int32_t syscall_getargs (uint8_t* buf, int32_t nbytes) {

    int i;

    // handle error condition
    if(buf==NULL || nbytes ==0 ){
      return -1;  
    }

    //create current pcb
    pcb_t * curr_pcb = get_current_pcb();

    /* should we check number of bytes? if we keep copying or return -1? */
 
    if(strlen(curr_pcb->args_array) <= nbytes){

        if(strlen(curr_pcb->args_array)==0){
        return -1;
    }else{
        // when we can use strcpy to copy entirely
        strcpy((int8_t*)buf,(int8_t*)(curr_pcb->args_array));
    }
    }else{
        // when we need to copy one by one
        for (i = 0; i < nbytes; i++) {
            buf[i] = curr_pcb->args_array[i];
        }
    }

    return 0;
}

/*
 *   syscall_vidmap
 *   DESCRIPTION: maps thevideo memory into user space
 *   INPUTS: screen_start -- pointer to screen_start pointer
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success, -1 on failure
 *   SIDE EFFECTS: none
 */

int32_t syscall_vidmap (uint8_t** screen_start) {
    if(screen_start < (uint8_t **)VID_PAGE_START || screen_start >= (uint8_t **)VID_PAGE_END)
    {
        return -1;
    }
    else
    {
        uint32_t terminal_id = get_current_pcb()->terminal_id;
        // Set up page table.
        page_table_program_vidmap[terminal_id][PT_ENTRY_IDX].present = 1;
        page_table_program_vidmap[terminal_id][PT_ENTRY_IDX].read_write = 1;
        page_table_program_vidmap[terminal_id][PT_ENTRY_IDX].user_supervisor = 1;
        page_table_program_vidmap[terminal_id][PT_ENTRY_IDX].write_through = 0;
        page_table_program_vidmap[terminal_id][PT_ENTRY_IDX].cache_disabled = 0;
        page_table_program_vidmap[terminal_id][PT_ENTRY_IDX].accessed = 0;
        page_table_program_vidmap[terminal_id][PT_ENTRY_IDX].dirty = 0;
        page_table_program_vidmap[terminal_id][PT_ENTRY_IDX].pt_attribute_index = 0;
        page_table_program_vidmap[terminal_id][PT_ENTRY_IDX].global_page = 0;
        page_table_program_vidmap[terminal_id][PT_ENTRY_IDX].available = 0;
        page_table_program_vidmap[terminal_id][PT_ENTRY_IDX].page_base_address = VIDEO >> VAL_12;

        // Modify processs page directory.
        uint32_t pid = get_current_pcb()->pid;
        page_directory_program[pid][PD_ENTRY_IDX].entry_PT.present = 1;
        page_directory_program[pid][PD_ENTRY_IDX].entry_PT.read_write = 1;
        page_directory_program[pid][PD_ENTRY_IDX].entry_PT.user_supervisor = 1; // user privilege
        page_directory_program[pid][PD_ENTRY_IDX].entry_PT.write_through = 0;
        page_directory_program[pid][PD_ENTRY_IDX].entry_PT.cache_disabled = 0;
        page_directory_program[pid][PD_ENTRY_IDX].entry_PT.accessed = 0;
        page_directory_program[pid][PD_ENTRY_IDX].entry_PT.reserved = 0;
        page_directory_program[pid][PD_ENTRY_IDX].entry_PT.page_size = 0; // 4KB page table
        page_directory_program[pid][PD_ENTRY_IDX].entry_PT.global_page = 0;
        page_directory_program[pid][PD_ENTRY_IDX].entry_PT.available = 0;
        page_directory_program[pid][PD_ENTRY_IDX].entry_PT.pt_base_address = (uint32_t)page_table_program_vidmap[terminal_id] >> VAL_12;

        load_page_directory(page_directory_program[pid]);

        // Calculate address.
        *screen_start = (uint8_t*)(PD_ENTRY_IDX * VAL_4 * VAL_1024 * VAL_1024 + PT_ENTRY_IDX * VAL_4 * VAL_1024);

        return  0;
    }
}

int32_t syscall_set_handler (int32_t signum, void* handler) {
    return -1;
}

int32_t syscall_sigreturn (void) {
    return -1;
}
