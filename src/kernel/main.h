#ifndef KERNEL_H
#define KERNEL_H

#include <stddef.h>

#include "memory.h"

struct boot_info {
  uint32_t num_memory_map_entries;
  uint32_t memory_map_base;
  uint32_t avail_low_mem_start;
  uint32_t avail_low_mem_end;
} __attribute__((packed));

extern "C" void kmain(boot_info *boot);

#endif
