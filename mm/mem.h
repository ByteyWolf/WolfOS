#ifndef MEM_H
#define MEM_H

#include "../boot/bootscheme.h"
#include <stdint.h>

void init_memory_manager(boot_data_t* mbd, void* kernel_start, void* kernel_end);

#endif
