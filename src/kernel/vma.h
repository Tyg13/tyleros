#ifndef VMA_H
#define VMA_H

#include <stddef.h>

namespace vma {
void init();
void *get_virtual_pages(size_t size);
void free_virtual_pages(void *address, size_t size);
void remove_from_free_list(void *base, size_t size);
void dump_free_list();
} // namespace vma

#endif
