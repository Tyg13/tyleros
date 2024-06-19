#include "low_memory_allocator.h"

#include <string.h>

namespace low_memory {

static uintptr_t avail_low_mem_start = 0;
static uintptr_t avail_low_mem_end = 0;

void init(uintptr_t avail_low_mem_start, uintptr_t avail_low_mem_end) {
  low_memory::avail_low_mem_start = avail_low_mem_start;
  low_memory::avail_low_mem_end = avail_low_mem_end;
}

void *allocate(size_t size) {
  if (avail_low_mem_start >= avail_low_mem_end) {
    return nullptr;
  }
  const auto base = reinterpret_cast<void *>(avail_low_mem_start);
  memset_v(base, 0, size);
  avail_low_mem_start += size;
  return base;
}

} // namespace low_memory
