#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

using page_table       = volatile uintptr_t[512];
using page_directory_table = volatile uintptr_t[512];
using page_directory_pointer_table = volatile uintptr_t[512];
using page_map_level_4_table = volatile uintptr_t *;

void init_paging();

void map_page(void * physical_page, void * virtual_page);
void unmap_page(void * virtual_page);

void * map_one_page();

#endif
