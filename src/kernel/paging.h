#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <stddef.h>

using page_level = volatile uintptr_t[512];

void init_paging(uintptr_t avail_low_mem_start, uintptr_t avail_low_mem_end);

void map_range(uintptr_t physical_start,
               uintptr_t physical_end,
               uintptr_t virtual_start,
               uintptr_t virtual_end);
void map_range_size(uintptr_t physical_start,
                    uintptr_t virtual_start,
                    size_t size);
void map_page(uintptr_t physical_page,
              uintptr_t virtual_page);
void unmap_page(uintptr_t virtual_page);

void * map_one_page();

inline void invlpg(uintptr_t page) {
    asm volatile("invlpg (%0)" :: "b"(page) : "memory");
}

inline void flush_tlb() {
    uintptr_t dummy;
    asm volatile ("mov %%cr3, %0" : "=a"(dummy));
    asm volatile ("mov %0, %%cr3" :: "a"(dummy));
}

#endif
