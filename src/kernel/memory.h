#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>

struct memory_map_entry {
    enum type : uint32_t {
        usable      = 1,
        reserved    = 2,
        reclaimable = 3,
        nvs         = 4,
        badmemory   = 5,
    };
    uintptr_t base;
    uint64_t  length;
    type      type;
    uint32_t  extended_attributes = 1;
}
__attribute((packed));

using memory_map = memory_map_entry[];

inline auto   g_num_of_memory_map_entries = * reinterpret_cast<uint32_t   *>(0x1000);
inline auto & g_memory_map                = * reinterpret_cast<memory_map *>(0x1008);

// 4KiB pages
constexpr static int PAGE_SIZE = 0x1000;

void init_memory();

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
