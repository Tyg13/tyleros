#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>

struct memory_map_entry {
    enum class type : uint32_t {
        usable      = 1,
        reserved    = 2,
        reclaimable = 3,
        nvs         = 4,
        badmemory   = 5,
    };
    char *    base;
    uint64_t  length;
    type      type;
    uint32_t  extended_attributes = 1;
}
__attribute((packed));

using page_table       = volatile uintptr_t[512];
using page_directory_table = volatile uintptr_t[512];
using page_directory_pointer_table = volatile uintptr_t[512];
using page_map_level_4_table = volatile uintptr_t *;
constexpr static auto PAGE_LEVEL_SIZE = 512 * sizeof(void *);

void init_memory();

void * kmmap(size_t n);
void kunmap(void *);

void * kmalloc(size_t count);
void kfree(void *);

constexpr static auto PAGE_PRESENT  = 1 << 0;
constexpr static auto PAGE_WRITE    = 1 << 1;

extern char __KERNEL_START__;
extern char __KERNEL_END__;

#endif
