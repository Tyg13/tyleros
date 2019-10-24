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
constexpr static auto PAGE_LEVEL_SIZE = 512 * sizeof(uintptr_t);

void init_memory();

void * kmmap(size_t n);
void kunmap(void *);

void * kmalloc(size_t count);
void kfree(void *);

constexpr static auto PAGE_PRESENT  = 1 << 0;
constexpr static auto PAGE_WRITE    = 1 << 1;

extern char __KERNEL_VMA_START__;
extern char __KERNEL_VMA_END__;
extern char __KERNEL_LMA_START__;
extern char __KERNEL_LMA_END__;

const ptrdiff_t KERNEL_VMA_OFFSET = &__KERNEL_VMA_START__ - &__KERNEL_LMA_START__;

#endif
