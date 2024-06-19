#ifndef PMA_H
#define PMA_H

#include <stddef.h>
#include <stdint.h>

namespace pma {
void init();
void *get_physical_page();
void free_physical_page(void *page);

extern bool physical_memory_allocator_available;

uintptr_t *get_base_of_page_stack();
size_t get_size_of_page_stack();
} // namespace pma

#endif
