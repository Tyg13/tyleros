#ifndef PMA_H
#define PMA_H

void init_physical_memory_allocator();
void * get_physical_page();
void free_physical_page(void * page);

extern bool physical_memory_allocator_available;

#endif
