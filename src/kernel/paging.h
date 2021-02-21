#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <stddef.h>

using page_table       = volatile uintptr_t[512];
using page_directory_table = volatile uintptr_t[512];
using page_directory_pointer_table = volatile uintptr_t[512];
using page_map_level_4_table = volatile uintptr_t *;

void init_paging();

void map_range(uintptr_t physical_start, uintptr_t physical_end, uintptr_t virtual_start, uintptr_t virtual_end);
void map_range_size(uintptr_t physical_start, uintptr_t virtual_start, size_t size);
void map_page(uintptr_t physical_page, uintptr_t virtual_page);
void unmap_page(uintptr_t virtual_page);

void * map_one_page();

#endif
