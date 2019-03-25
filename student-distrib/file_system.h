#ifndef _FILE_SYSTEM_H_
#define _FILE_SYSTEM_H_

#include "types.h"

#ifndef ASM

#define FILE_NAME_MAX_LENGTH 32

// Constants for file types.
#define RTC_FILE 0
#define DIRECTORY_FILE 1
#define REGULAR_FILE 2

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

// Three helper routines that actually interacts with file system.
extern int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry);
extern int32_t read_dentry_by_index (uint32_t index, dentry_t* dentry);
extern int32_t read_data (uint32_t inode, uint32_t offset, 
                                uint8_t* buf, uint32_t length);

// File system interfaces.
extern int file_system_init(uint32_t base_address);

extern int32_t file_open(const uint8_t *filename);
extern int32_t file_close(int32_t fd);
extern int32_t file_read(int32_t fd, void *buf, int32_t nbytes);
extern int32_t file_write(int32_t fd, void *buf, int32_t nbytes);

extern int32_t directory_open(const uint8_t *filename);
extern int32_t directory_close(int32_t fd);
extern int32_t directory_read(int32_t fd, void *buf, int32_t nbytes);
extern int32_t directory_write(int32_t fd, void *buf, int32_t nbytes);

#endif

#endif