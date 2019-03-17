#ifndef _FILE_SYSTEM_H_
#define _FILE_SYSTEM_H_

#include "types.h"

#ifndef ASM

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