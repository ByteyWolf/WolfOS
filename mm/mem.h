#ifndef MEM_H
#define MEM_H

#include "../util/multiboot.h"
#include <stdint.h>

void init_memory_manager(multiboot_info_t* mbd, void* kernel_start, void* kernel_end);

#endif
