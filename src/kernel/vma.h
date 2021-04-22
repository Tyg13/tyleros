#ifndef VMA_H
#define VMA_H

#include <stddef.h>

void init_virtual_memory_allocator();
void * get_virtual_pages(size_t size);
void free_virtual_pages(void * address, size_t size);

#endif
