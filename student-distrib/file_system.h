#ifndef _FILE_SYSTEM_H_
#define _FILE_SYSTEM_H_

#include "types.h"

#ifndef ASM

#define FILE_NAME_MAX_LENGTH 32

// Constants that actually make no sense but just to
//  eliminate magic numbers.
#define VAL_32 32
#define VAL_24 24
#define VAL_52 52
#define VAL_63 63
#define VAL_1023 1023
#define VAL_4096 4096

// Constants for file types.
#define RTC_FILE 0
#define DIRECTORY_FILE 1
#define REGULAR_FILE 2

// Structs corresponding to file system specification.
typedef struct dentry {
    uint8_t file_name[VAL_32];
    uint32_t file_type;
    uint32_t inode_idx;
    uint8_t reserved[VAL_24];
} dentry_t;

typedef struct boot_block {
    uint32_t num_dentry;
    uint32_t num_inode;
    uint32_t num_data_block;
    uint8_t reserved[VAL_52];
    dentry_t dentry[VAL_63];
} boot_block_t;

typedef struct inode {
    uint32_t length;
    uint32_t date_block_idx[VAL_1023];
} inode_t;

typedef struct data_block {
    uint8_t data[VAL_4096];
} data_block_t;

// Three helper routines that actually interacts with file system.
extern int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry);
extern int32_t read_dentry_by_index (uint32_t index, dentry_t* dentry);
extern int32_t read_data (uint32_t inode, uint32_t offset, 
                                uint8_t* buf, uint32_t length);

// File system interfaces.
extern int file_system_init(uint32_t base_address);

// File open/close/read/write
extern int32_t file_open(const uint8_t *filename);
extern int32_t file_close(int32_t fd);
extern int32_t file_read(int32_t fd, void *buf, int32_t nbytes);
extern int32_t file_write(int32_t fd, void *buf, int32_t nbytes);

// Directory open/close/read/write
extern int32_t directory_open(const uint8_t *filename);
extern int32_t directory_close(int32_t fd);
extern int32_t directory_read(int32_t fd, void *buf, int32_t nbytes);
extern int32_t directory_write(int32_t fd, void *buf, int32_t nbytes);

// Check if an executable exists and has correct magic number.
// Return value: 0 not exist or magic number is not correct.
//               1 exist and magic number is correct.
extern int32_t check_executable(const uint8_t *filename);

// Load the executable into memory.
// Return value: program entry address or -1 on error.
extern int32_t load_executable(const uint8_t *filename);

#endif

#endif
