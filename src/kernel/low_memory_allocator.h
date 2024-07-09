#ifndef LOW_MEMORY_ALLOCATOR_H
#define LOW_MEMORY_ALLOCATOR_H

#include "util.h"

#include <stddef.h>
#include <stdint.h>

namespace low_memory {

void init(uintptr_t low_mem_start, uintptr_t low_mem_end);
void *allocate(size_t size, kstd::Align align = kstd::Align());
template <typename T> T *allocate() {
  return (T *)allocate(sizeof(T), kstd::align_of<T>);
}

uintptr_t get_avail_low_mem_start();
uintptr_t get_avail_low_mem_end();

} // namespace low_memory

#endif
