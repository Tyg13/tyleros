#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>

extern char __KERNEL_VMA_START__;
extern char __KERNEL_VMA_END__;
extern char __KERNEL_LMA_START__;
extern char __KERNEL_LMA_END__;

namespace memory {
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

    // 4KiB pages
    constexpr static auto PAGE_SIZE = 0x1000;

    void init(uint32_t memory_map_base,
              uint32_t num_memory_map_entries,
              uint32_t avail_low_mem_start,
              uint32_t avail_low_mem_end);
    memory_map& get_memory_map();

    void * alloc(size_t count);
    void free(void *);

    constexpr static auto PAGE_PRESENT  = 1 << 0;
    constexpr static auto PAGE_WRITE    = 1 << 1;

    const ptrdiff_t KERNEL_VMA_OFFSET = &__KERNEL_VMA_START__ - &__KERNEL_LMA_START__;

    struct early_bump_allocator {
        static constexpr auto MAX_SIZE = 0x100000;
        uintptr_t m_base;
        size_t    m_size = 0;

        void * allocate_pages(size_t num_pages);
        static early_bump_allocator * get();
    };
}

#endif
