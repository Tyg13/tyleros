#ifndef PAGING_H
#define PAGING_H

#include <stddef.h>
#include <stdint.h>

namespace paging {

void early_init();
void finish_init();

enum class attributes : uint64_t {
  NONE = 0,
  // If set, the page is actually in physical memory. If unset, it may not be
  // (e.g. swapped out)
  PRESENT = 1ULL << 0,
  // If set, the page is read/write; otherwise, it is read-only.
  RW = 1ULL << 1,
  // If set, the page is not executable.
  XD = 1ULL << 63,
};
inline uint64_t operator&(uint64_t a, attributes b) { return a & (uint64_t)b; }
inline uint64_t operator&(attributes a, uint64_t b) { return (uint64_t)a & b; }
inline uint64_t operator|(uint64_t a, attributes b) { return a | (uint64_t)b; }
inline uint64_t operator|(attributes a, uint64_t b) { return (uint64_t)a | b; }
inline uint64_t operator~(attributes a) { return ~(uint64_t)a; }
inline attributes operator|(attributes a, attributes b) {
  return (attributes)((uint64_t)a | (uint64_t)b);
}

static constexpr auto NUM_PAGE_TABLE_ENTRIES = 512;
using page_level = volatile uintptr_t[NUM_PAGE_TABLE_ENTRIES];
using page_level_allocator = void*();

struct page_tables {
  page_level *base = nullptr;
  page_level_allocator *allocate_page_level = nullptr;

  void *map_range(uintptr_t physical_start, uintptr_t physical_end,
                  uintptr_t virtual_start, uintptr_t virtual_end);
  void *map_range_size(uintptr_t physical_start, uintptr_t virtual_start,
                       size_t size);
  void *map_page(uintptr_t physical_page, uintptr_t virtual_page,
                 attributes attrs = attributes::PRESENT | attributes::RW);
  void unmap_page(uintptr_t virtual_page);
  void *identity_map_page(uintptr_t addr);
  void *identity_map_range_size(uintptr_t addr, size_t size);

  uintptr_t get_physical_address(void *virtual_addr) const;

  void dump() const;
};

extern page_tables kernel_page_tables;

inline void invlpg(uintptr_t page) {
  asm volatile("invlpg (%0)" ::"r"(page) : "memory");
}
} // namespace paging

#endif
