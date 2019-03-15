#include "file_system.h"

#include "lib.h"

static uint32_t file_system_base_address = NULL;

int file_system_init(uint32_t base_address) {
    if(base_address == NULL)
        return -1;

    file_system_base_address = base_address;

    return 0;
}