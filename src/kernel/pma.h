#ifndef PMA_H
#define PMA_H

#include <stdint.h>
#include <stddef.h>

void init_physical_memory_allocator();
void * get_physical_page();
void free_physical_page(void * page);

extern bool physical_memory_allocator_available;

uintptr_t * get_base_of_page_stack();
size_t get_size_of_page_stack();

#endif
