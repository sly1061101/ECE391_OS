#include "file_system.h"

#include "lib.h"

#define DENTRY_START_OFFSET 64

// Structs corresponding to file system specification.
typedef struct dentry {
    uint8_t file_name[32];
    uint32_t file_type;
    uint32_t inode_idx;
    uint8_t reserved[24];
} dentry_t;

typedef struct boot_block {
    uint32_t num_dentry;
    uint32_t num_inode;
    uint32_t num_data_block;
    uint8_t reserved[52];
    dentry_t dentry[63];
} boot_block_t;

typedef struct inode {
    uint32_t length;
    uint32_t date_block_idx[1023];
} inode_t;

typedef struct data_block {
    uint8_t data[4096];
} data_block_t;

// Starting address for file system in kernel memory.
static uint32_t file_system_base_address = NULL;
// Pointer to boot block in file system.
static boot_block_t *boot_block = NULL;

// Three helper routines that actually interacts with file system.
static int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry);
static int32_t read_dentry_by_index (uint32_t index, dentry_t* dentry);
static int32_t read_data (uint32_t inode, uint32_t offset, 
                                uint8_t* buf, uint32_t length);

int32_t read_dentry_by_name(const uint8_t *fname, dentry_t *dentry) {
    if(file_system_base_address == NULL)
        return -1;
    
    if(strlen(fname) > FILE_NAME_MAX_LENGTH)
        return -1;

    dentry_t *source;

    int i;
    for(i = 0; i < boot_block->num_dentry; ++i) {
        source = (dentry_t *)((char *)file_system_base_address 
                                + DENTRY_START_OFFSET + i * sizeof(dentry_t));
        if(strncmp((uint8_t *)fname, (uint8_t *)source->file_name, 
                                                FILE_NAME_MAX_LENGTH) == 0) {
            read_dentry_by_index(i, dentry);
            return 0;
        }
    }

    return -1;
}

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

int32_t file_open(const uint8_t *filename) {
    if(filename == NULL)
        return -1;
    
    dentry_t dentry;
    if(read_dentry_by_name(filename, &dentry) == -1)
        return -1;
    
    // If file_type is not regular file.
    if(dentry.file_type != REGULAR_FILE)
        return -1;
    
    memcpy(&dentry_opened_file, &dentry, sizeof(dentry_t));

    opened_file_offset = 0;

    has_file_opened = 1;
    
    return 0;
}

int32_t file_close(int32_t fd) {
    if(!has_file_opened)
        return -1;
    
    has_file_opened = 0;

    return 0;
}

int32_t file_read(int32_t fd, void *buf, int32_t nbytes) {
    if(!has_file_opened)
        return -1;
    
    int ret = read_data(dentry_opened_file.inode_idx, opened_file_offset, buf, nbytes);

    if(ret == -1)
        return -1;
    
    opened_file_offset += ret;

    return ret;
}

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
        *((char *)buf + i) = dentry.file_name[i];
        i++;
        // If last copied character is end-of-line, stop here.
        if(dentry.file_name[i - 1] == '\0')
            break;
    }

    opened_directory_offset++;

    return i;
}

int32_t directory_close(int32_t fd) {
    if(!has_directory_opened)
        return -1;
    
    has_directory_opened = 0;

    return 0;
}

int32_t directory_write(int32_t fd, void *buf, int32_t nbytes) {
    return -1;
}

// File system initialization function.
int file_system_init(uint32_t base_address) {
    if(base_address == NULL)
        return -1;

    file_system_base_address = base_address;
    boot_block = (boot_block_t *)file_system_base_address;

    has_file_opened = 0;
    has_directory_opened = 0;

    return 0;
}