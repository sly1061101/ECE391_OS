#include "file_system.h"

#include "lib.h"

static uint32_t file_system_base_address = NULL;

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

static int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry);
static int32_t read_dentry_by_index (uint32_t index, dentry_t* dentry);
static int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);

int32_t read_dentry_by_name(const uint8_t *fname, dentry_t *dentry) {
    if(file_system_base_address == NULL)
        return -1;

    boot_block_t *boot_block = (boot_block_t *)file_system_base_address;
    dentry_t *source;

    int i;
    for(i = 0; i < boot_block->num_dentry; ++i) {
        source = (dentry_t *)((char *)file_system_base_address + 64 + i * sizeof(dentry_t));
        if(strncmp((uint8_t *)fname, (uint8_t *)source->file_name, 32) == 0) {
            read_dentry_by_index(i, dentry);
            return 0;
        }
    }

    return -1;
}

int32_t read_dentry_by_index(uint32_t index, dentry_t *dentry) {
    if(file_system_base_address == NULL)
        return -1;

    boot_block_t *boot_block = (boot_block_t *)file_system_base_address;
    if(index >= boot_block->num_dentry)
        return -1;

    dentry_t *source = (dentry_t *)((char *)file_system_base_address + 64 + index * sizeof(dentry_t));
    memcpy((void *)dentry, (void *)source, sizeof(dentry_t));

    return 0;
}

int32_t read_data(uint32_t inode_index, uint32_t offset, uint8_t *buf, uint32_t length) {
    if(file_system_base_address == NULL)
        return -1;

    boot_block_t *boot_block = (boot_block_t *)file_system_base_address;
    if(inode_index >= boot_block->num_inode)
        return -1;

    const inode_t *inode= (inode_t *)((char *)file_system_base_address + sizeof(boot_block_t) + inode_index * sizeof(inode_t));

    int byte_count;

    int curr_byte;
    uint32_t cur_data_block_idx;
    data_block_t *cur_data_block;
    for(byte_count = 0, curr_byte = offset; byte_count < length && curr_byte < inode->length; ++byte_count, ++curr_byte) {
        // Update current data block at the beginning and whenever enters a new block.
        if(byte_count == 0 || curr_byte % sizeof(data_block_t) == 0) {
            cur_data_block_idx = inode->date_block_idx[curr_byte / sizeof(data_block_t)];
            if(cur_data_block_idx > boot_block->num_data_block)
                return -1;
            cur_data_block = (data_block_t *)((char *)file_system_base_address + sizeof(boot_block_t) + (boot_block->num_inode) * sizeof(inode_t) + cur_data_block_idx * sizeof(data_block_t));
        }

        buf[byte_count] = cur_data_block->data[curr_byte % sizeof(data_block_t)];
    }

    return byte_count;
}

int file_system_init(uint32_t base_address) {
    if(base_address == NULL)
        return -1;

    file_system_base_address = base_address;

    return 0;
}