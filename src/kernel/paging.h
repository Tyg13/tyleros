#ifndef PAGING_H
#define PAGING_H

#include <stddef.h>
#include <stdint.h>

namespace paging {
using page_level = volatile uintptr_t[512];

void init();

void map_range(uintptr_t physical_start, uintptr_t physical_end,
               uintptr_t virtual_start, uintptr_t virtual_end);
void map_range_size(uintptr_t physical_start, uintptr_t virtual_start,
                    size_t size);
void map_page(uintptr_t physical_page, uintptr_t virtual_page);
void unmap_page(uintptr_t virtual_page);
void identity_map_page(uintptr_t addr);

void *map_one_page();

inline void invlpg(uintptr_t page) {
  asm volatile("invlpg (%0)" ::"b"(page) : "memory");
}

inline void flush_tlb() {
  uintptr_t dummy;
  asm volatile("mov %%cr3, %0" : "=r"(dummy));
  asm volatile("mov %0, %%cr3" ::"r"(dummy));
}
} // namespace paging

#endif
