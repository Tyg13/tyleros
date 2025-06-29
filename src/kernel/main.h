#ifndef KERNEL_H
#define KERNEL_H

#include <stddef.h>
#include <stdint.h>

struct boot_info {
  uint32_t num_memory_map_entries;
  uint32_t memory_map_base;
  uint32_t avail_low_mem_start;
  uint32_t avail_low_mem_end;
  uint32_t kernel_image_start;
  uint32_t kernel_image_size;
  uint32_t drive_number;
  uint32_t kernel_expected_crc32;
  uint64_t kernel_stack_base;
} __attribute__((packed));

extern "C" void kmain(const boot_info &boot);

#endif
