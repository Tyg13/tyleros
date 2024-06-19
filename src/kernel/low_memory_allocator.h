#ifndef LOW_MEMORY_ALLOCATOR_H
#define LOW_MEMORY_ALLOCATOR_H

#include <stddef.h>
#include <stdint.h>

namespace low_memory {

void init(uintptr_t avail_low_mem_start, uintptr_t avail_low_mem_end);
void *allocate(size_t size);

} // namespace low_memory

#endif
