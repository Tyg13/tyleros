#include "low_memory_allocator.h"
#include "debug.h"

#include <assert.h>
#include <string.h>

namespace low_memory {

static uintptr_t avail_low_mem_start = 0;
static uintptr_t avail_low_mem_end = 0;
//kstd::intrusive_bitmap *page_bitmap = nullptr;

uintptr_t get_avail_low_mem_start() { return avail_low_mem_start; }
uintptr_t get_avail_low_mem_end() { return avail_low_mem_end; }

void init(uintptr_t low_mem_start, uintptr_t low_mem_end) {
  avail_low_mem_start = low_mem_start;
  avail_low_mem_end = low_mem_end;
  assert(avail_low_mem_start < avail_low_mem_end);
  // page_bitmap = (kstd::intrusive_bitmap*)avail_low_mem_start;
  // const auto low_mem_pages =
  //     (avail_low_mem_end - avail_low_mem_start) / memory::PAGE_SIZE;
  // const auto size_of_page_bitmap =
  //     kstd::intrusive_bitmap::size_in_bytes(low_mem_pages);
  // const auto avail_low_mem_pages =
  //     (avail_low_mem_end - (avail_low_mem_start + size_of_page_bitmap)) /
  //     memory::PAGE_SIZE;
  //*page_bitmap = kstd::intrusive_bitmap{avail_low_mem_pages};
  debug::printf("Low memory allocator init : %lx bytes at 0x%p - 0x%p\n",
                (avail_low_mem_end - avail_low_mem_start),
                (void *)avail_low_mem_start, (void *)avail_low_mem_end);
}

void *allocate(size_t size, kstd::Align align) {
  const auto aligned_start = kstd::align_to(avail_low_mem_start, align);
  if (aligned_start >= (avail_low_mem_end - size)) {
    kstd::panic("Ran out of low memory!");
  }
  avail_low_mem_start = aligned_start;
  const auto base = reinterpret_cast<void *>(avail_low_mem_start);
  memset(base, 0, size);
  avail_low_mem_start += size;
  return base;
}

} // namespace low_memory
