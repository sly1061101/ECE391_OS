#include "file_system.h"
#include "process.h"

#include "lib.h"

#define DENTRY_START_OFFSET 64
#define PROGRAM_IMAGE_START_ADDRESS 0x08048000
#define USER_STACK_SIZE 0X400000

#define NUM_MAGIC_NUMBER 4
#define MAGIC_NUM_0 0x7f
#define MAGIC_NUM_1 0x45
#define MAGIC_NUM_2 0x4c
#define MAGIC_NUM_3 0x46
#define MAGIC_NUMBER_ADDRESS_OFFSET 24

// Starting address for file system in kernel memory.
static uint32_t file_system_base_address = NULL;
// Pointer to boot block in file system.
static boot_block_t *boot_block = NULL;

/*read_dentry_by_name
* DISCRIPTION: fill in the dentry t block passed as their second argument with the file name, file
               type, and inode number for the file, then return 0.
* INPUT:    const uint8_t fname
            dentry_t dentry
* OUTPUT: NONE
* RETURN VALUE: 0 on success, -1 on failure
* SIDE EFFECTS: return -1 on failure, indicating a non-existent file. 0 on success, indicating dentry t block passed as their second argument with the file name
*/

int32_t read_dentry_by_name(const uint8_t *fname, dentry_t *dentry) {
    if(file_system_base_address == NULL)
        return -1;
    
    if(strlen((int8_t*)fname) > FILE_NAME_MAX_LENGTH)
        return -1;

    dentry_t *source;

    int i;
    for(i = 0; i < boot_block->num_dentry; ++i) {
        source = (dentry_t *)((char *)file_system_base_address 
                                + DENTRY_START_OFFSET + i * sizeof(dentry_t));
        if(strncmp((int8_t*)fname, (int8_t*)source->file_name, 
                                                FILE_NAME_MAX_LENGTH) == 0) {
            read_dentry_by_index(i, dentry);
            return 0;
        }
    }

    return -1;
}

/*read_dentry_by_index
* DISCRIPTION: fill in the dentry t block passed as their second argument with the parameter index, file
               type, and inode number for the file, then return 0.
* INPUT:    uint32_t index
            dentry_t dentry
* OUTPUT: NONE
* RETURN VALUE: 0 on success, -1 on failure
* SIDE EFFECTS: return -1 on failure, indicating a invalid index. 0 on success, indicating dentry t block passed as their second argument with the parameter index.
*/

int32_t read_dentry_by_index(uint32_t index, dentry_t *dentry) {
    if(file_system_base_address == NULL)
        return -1;

    if(index >= boot_block->num_dentry)
        return -1;

    dentry_t *source = (dentry_t *)((char *)file_system_base_address 
                                        + DENTRY_START_OFFSET + index * sizeof(dentry_t));
    memcpy((void *)dentry, (void *)source, sizeof(dentry_t));

    return 0;
}

/*read_data
* DISCRIPTION: reading up to length bytes starting from position offset in the file with inode number inode and returning the number of bytes read and placed in the buffer
* INPUT:    uint8_t inode_index
            uint8_t offset
            uint8_t *buf
            uint8_t length
* OUTPUT: NONE
* RETURN VALUE: byte_count on success, -1 on failure
* SIDE EFFECTS: Same as DESCRIPTION.
*/

int32_t read_data(uint32_t inode_index, uint32_t offset, uint8_t *buf, uint32_t length) {
    if(file_system_base_address == NULL)
        return -1;

    if(inode_index >= boot_block->num_inode)
        return -1;

    const inode_t *inode= (inode_t *)((char *)file_system_base_address 
                                        + sizeof(boot_block_t) + inode_index * sizeof(inode_t));

    int byte_count;

    int curr_byte;
    uint32_t cur_data_block_idx;
    data_block_t *cur_data_block;
    for(byte_count = 0, curr_byte = offset; 
                byte_count < length && curr_byte < inode->length; ++byte_count, ++curr_byte) {
        // Update current data block at the beginning and whenever enters a new block.
        if(byte_count == 0 || curr_byte % sizeof(data_block_t) == 0) {
            cur_data_block_idx = inode->date_block_idx[curr_byte / sizeof(data_block_t)];
            if(cur_data_block_idx > boot_block->num_data_block)
                return -1;
            cur_data_block = 
                (data_block_t *)((char *)file_system_base_address + sizeof(boot_block_t) 
                                    + (boot_block->num_inode) * sizeof(inode_t) 
                                    + cur_data_block_idx * sizeof(data_block_t));
        }

        buf[byte_count] = cur_data_block->data[curr_byte % sizeof(data_block_t)];
    }

    return byte_count;
}

// Stuffs for file operations.
dentry_t dentry_opened_file;
int32_t has_file_opened;
uint32_t opened_file_offset;

/*file_open
* DISCRIPTION: perform type-specific initialization on file operations jump table.
* INPUT:    const uint8_t *filename
* OUTPUT: NONE
* RETURN VALUE: 0 on success, -1 on failure
* SIDE EFFECTS: Same as DESCRIPTION.
*/

int32_t file_open(const uint8_t *filename) {
    if(filename == NULL)
        return -1;
    
    dentry_t dentry;
    if(read_dentry_by_name(filename, &dentry) == -1)
        return -1;
    
    // If file_type is not regular file.
    if(dentry.file_type != REGULAR_FILE)
        return -1;
    
    return 0;
}

/*file_close
* DISCRIPTION: perform type-specific close on file operations jump table.
* INPUT:    int32_t fd
* OUTPUT: NONE
* RETURN VALUE: 0 if file is closed and open later, -1 if file is already closed.
* SIDE EFFECTS: Close file table
*/

int32_t file_close(int32_t fd) {
    return 0;
}

/*file_read
* DISCRIPTION: perform type-specific read on file operations jump table.
* INPUT:    int32_t fd
            int32_t nbytes
            void* buf
* OUTPUT: NONE
* RETURN VALUE: -1 if read operation is unsuccessful, ret if read data is successful
* SIDE EFFECTS: read data from file table
*/

int32_t file_read(int32_t fd, void *buf, int32_t nbytes) {
    pcb_t *pcb = get_current_pcb();
    
    int ret = read_data(pcb->file_array[fd].inode, pcb->file_array[fd].file_position, buf, nbytes);

    printf("DEBUG: fd = %d, nbytes = %d, ret = %d\n", fd, nbytes, ret);

    if(ret == -1)
        return -1;
    
    pcb->file_array[fd].file_position += ret;

    return ret;
}

/*file_write
* DISCRIPTION: perform type-specific write on file operations jump table.
* INPUT:    int32_t fd
            int32_t nbytes
            void* buf
* OUTPUT: NONE
* RETURN VALUE: -1 if write operation is unsuccessful, 0 if writing operation is successful
* SIDE EFFECTS: write data to file table
*/

int32_t file_write(int32_t fd, void *buf, int32_t nbytes) {
    return -1;
}

// Stuffs for directory operations.
dentry_t dentry_opened_directory;
int32_t has_directory_opened;
uint32_t opened_directory_offset;

int32_t directory_open(const uint8_t *filename) {
    if(filename == NULL)
        return -1;
    
    dentry_t dentry;
    if(read_dentry_by_name(filename, &dentry) == -1)
        return -1;
    
    // If file_type is not directory.
    if(dentry.file_type != DIRECTORY_FILE)
        return -1;
    
    memcpy(&dentry_opened_directory, &dentry, sizeof(dentry_t));

    opened_directory_offset = 0;

    has_directory_opened = 1;
    
    return 0;
}

/*directroy_read
* DISCRIPTION:  read files filename by filename, including “.”
* INPUT:    int32_t fd
            int32_t nbytes
            void* buf
* OUTPUT: NONE
* RETURN VALUE: -1 if write operation is unsuccessful, index of data in dentry if read is successful.
* SIDE EFFECTS: write data to file table
*/

int32_t directory_read(int32_t fd, void *buf, int32_t nbytes) {
    if(!has_directory_opened)
        return -1;
    
    // Return 0 represents EOI.
    if(opened_directory_offset >= boot_block->num_dentry)
        return 0;

    dentry_t dentry;
    read_dentry_by_index(opened_directory_offset, &dentry);

    int i = 0;
    // Maximum filename length is 32 according to filesystem specification.
    while(i < nbytes && i < FILE_NAME_MAX_LENGTH) {
        // If has reached end-of-line, stop here.
        if(dentry.file_name[i] == '\0')
            break;

        *((char *)buf + i) = dentry.file_name[i];
        i++;
    }

    opened_directory_offset++;

    return i;
}

/*directroy_close
* DISCRIPTION:  close directory and return 0;
* INPUT:    int32_t fd
* OUTPUT: NONE
* RETURN VALUE: -1 if write operation is unsuccessful, index of data in dentry if read is successful.
* SIDE EFFECTS: return -1 if directory is already closed, return 0 if closed later manually.
*/

int32_t directory_close(int32_t fd) {
    if(!has_directory_opened)
        return -1;
    
    has_directory_opened = 0;

    return 0;
}

/*directroy_write
* DISCRIPTION:  do nothing and return -1.
* INPUT:    int32_t fd
            int32_t nbytes
            void* buf
* OUTPUT: NONE
* RETURN VALUE: -1.
* SIDE EFFECTS: return -1.
*/

int32_t directory_write(int32_t fd, void *buf, int32_t nbytes) {
    return -1;
}

/*file_system_init
* DISCRIPTION:  initialize file system
* INPUT:        uint32_t base_address
* OUTPUT: NONE
* RETURN VALUE: 0
* SIDE EFFECTS: initialize file system.
*/
int file_system_init(uint32_t base_address) {
    if(base_address == NULL)
        return -1;

    file_system_base_address = base_address;
    boot_block = (boot_block_t *)file_system_base_address;

    has_file_opened = 0;
    has_directory_opened = 0;

    return 0;
}

/*check_executable
* DISCRIPTION: Check if an executable exists and has correct magic number.
* INPUT:    const uint8_t *filename -- executable name to be checked.
* OUTPUT: NONE
* RETURN VALUE: 1 if pass, 0 if fail.
* SIDE EFFECTS: none.
*/
int32_t check_executable(const uint8_t *filename) {
    dentry_t dentry;

    if(read_dentry_by_name(filename, &dentry) != 0)
        return 0;

    if(dentry.file_type != REGULAR_FILE)
        return 0;

    uint8_t magic_number[NUM_MAGIC_NUMBER];

    if(read_data(dentry.inode_idx, 0, magic_number, NUM_MAGIC_NUMBER) != NUM_MAGIC_NUMBER)
        return 0;

    if(magic_number[0] != MAGIC_NUM_0 || magic_number[1] != MAGIC_NUM_1 
        || magic_number[2] != MAGIC_NUM_2 || magic_number[3] != MAGIC_NUM_3) {
        return 0;
    }
    
    return 1;
}

/*load_executable
* DISCRIPTION: Load the executable into memory..
* INPUT:    const uint8_t *filename -- executable name to be loaded.
* OUTPUT: NONE
* RETURN VALUE: 0 if success, -1 if error.
* SIDE EFFECTS: none.
*/
extern int32_t load_executable(const uint8_t *filename) {
    if(!check_executable(filename))
        return -1;

    dentry_t dentry;

    if(read_dentry_by_name(filename, &dentry) != 0)
        return -1;

    if(dentry.file_type != REGULAR_FILE)
        return -1;

    // Get the entry address.
    uint32_t entry_address;

    if(read_data(dentry.inode_idx, MAGIC_NUMBER_ADDRESS_OFFSET, (uint8_t *)&entry_address, NUM_MAGIC_NUMBER) != NUM_MAGIC_NUMBER)
        return -1;

    // Load entire program image into memory.
    // TODO: Use the file size information in inode check.
    if(read_data(dentry.inode_idx, 0, (uint8_t *)PROGRAM_IMAGE_START_ADDRESS, USER_STACK_SIZE) == -1)
        return -1;
    
    return entry_address;
}
