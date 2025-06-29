#ifndef PMA_H
#define PMA_H

#include <stddef.h>
#include <stdint.h>

namespace pma {
void early_init();
void finish_init();
uintptr_t get_physical_page();
uintptr_t get_contiguous_physical_pages(size_t n);
void free_physical_page(void *page);

extern bool physical_memory_allocator_available;
} // namespace pma

#endif
