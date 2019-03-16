#include "file_system.h"

#include "lib.h"

static uint32_t file_system_base_address = NULL;

typedef struct boot_block {
    uint32_t num_dentry;
    uint32_t num_inode;
    uint32_t num_data_block;
    uint8_t reserved[52];
} boot_block_t;

typedef struct dentry {
    uint8_t file_name[32];
    uint32_t file_type;
    uint32_t inode_num;
    uint8_t reserved[24];
} dentry_t;

static int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry);
static int32_t read_dentry_by_index (uint32_t index, dentry_t* dentry);

int32_t read_dentry_by_name(const uint8_t *fname, dentry_t *dentry) {
    if(file_system_base_address == NULL)
        return -1;

    boot_block_t *boot_block = (boot_block_t *)file_system_base_address;
    dentry_t *source;

    int i;
    for(i = 0; i < boot_block->num_dentry; ++i) {
        source = (dentry_t *)((char *)file_system_base_address + sizeof(boot_block_t) + i * sizeof(dentry_t));
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

    if(index >= 63)
        return -1;

    dentry_t *source = (dentry_t *)((char *)file_system_base_address + sizeof(boot_block_t) + index * sizeof(dentry_t));
    memcpy((void *)dentry, (void *)source, sizeof(dentry_t));

    return 0;
}

int file_system_init(uint32_t base_address) {
    if(base_address == NULL)
        return -1;

    file_system_base_address = base_address;

    return 0;
}