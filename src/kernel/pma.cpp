#include "pma.h"

#include "memory.h"
#include "paging.h"
#include "panic.h"
#include "util.h"
#include "vma.h"

#include "libadt/intrusive_bitmap.h"

#include <assert.h>

namespace pma {

static uint64_t max_pages_total = 0;
static uintptr_t physical_page_base = 0;
static adt::bitmap *physical_page_map = nullptr;
static constexpr bool FREE = false;
static constexpr bool OCCUPIED = true;

void early_init() {
  using memory::PAGE_SIZE;
  using memory::PAGE_ALIGN;

  const auto &largest_usable_memory_map_entry = memory::get_memory_map()[0];
  max_pages_total = largest_usable_memory_map_entry.length / PAGE_SIZE;
   // Page align to bottom of largest available memory region
  physical_page_map = (adt::bitmap *)kstd::align_to(
      largest_usable_memory_map_entry.base, PAGE_ALIGN);
  // Identity map the pages needed for the page bitmap itself
  const auto pages_needed_for_page_map =
      kstd::div_ceil(adt::bitmap::size_required(max_pages_total), PAGE_SIZE);
  //const auto pages_in_page_map = max_pages_total - pages_needed_for_page_map;
  physical_page_map =
      paging::kernel_page_tables.identity_map_pages_into_kernel_space(
          physical_page_map, pages_needed_for_page_map);
  adt::bitmap::make(*physical_page_map, max_pages_total);

  physical_page_base = (uintptr_t)physical_page_map +
                       pages_needed_for_page_map * PAGE_SIZE -
                       memory::KERNEL_VMA_OFFSET;

  //printf("pma : initialized %lu page(s) at 0x%p\n", pages_in_page_map,
  //       (void*)physical_page_base);
}

void finish_init() {
  vma::remove_from_free_list(
      physical_page_map,
      kstd::align_to((uintptr_t)physical_page_base + physical_page_map->size(),
                     memory::PAGE_ALIGN));
}

uintptr_t get_physical_page() {
  const auto new_page_idx = physical_page_map->find_first(FREE);
  if (new_page_idx == -1)
    kstd::panic("Allocation failed, out of physical pages!");

  physical_page_map->set(new_page_idx);
  const auto new_page = new_page_idx * memory::PAGE_SIZE + physical_page_base;
  assert((void *)new_page && "Top of page stack is null!");
  return new_page;
}

uintptr_t get_contiguous_physical_pages(size_t n) {
  const auto new_pages_idx = physical_page_map->find_first_n(FREE, n);
  if (new_pages_idx == -1)
    kstd::panic("Allocation failed, can't find %zx contiguous physical pages!",
                n);
  for (unsigned i = 0; i < n; ++i)
    physical_page_map->set(new_pages_idx + i);

  const auto new_pages = new_pages_idx * memory::PAGE_SIZE + physical_page_base;
  assert((void *)new_pages && "Top of page stack is null!");
  return new_pages;
}

void free_physical_page(void *page) {
  assert(page && "Page being freed is null!");
  const auto page_idx =
      ((uintptr_t)page - physical_page_base) / memory::PAGE_SIZE;
  assert(physical_page_map->test(page_idx) &&
         "page being freed was never allocated!");
  physical_page_map->reset(page_idx);
}
} // namespace pma

//void init_page_stack() {
//static uintptr_t *top_of_page_stack = nullptr;
//static uintptr_t *base_of_page_stack = nullptr;
//static uint64_t max_pages_total = 0;
//static uint64_t pages_needed_for_page_stack = 0;
//
//  using memory::PAGE_SIZE;
//  using memory::PAGE_ALIGN;
//
//  const auto &largest_usable_memory_map_entry = memory::get_memory_map()[0];
//  max_pages_total = largest_usable_memory_map_entry.length / PAGE_SIZE;
//
//  // Page align page stack at bottom of largest available memory region
//  base_of_page_stack = (uintptr_t *)kstd::align_to(
//      largest_usable_memory_map_entry.base, PAGE_ALIGN);
//  top_of_page_stack = reinterpret_cast<uintptr_t *>(base_of_page_stack);
//
//  // Identity map pages needed for the page stack itself
//  const auto size_of_page_stack = sizeof(uintptr_t) * max_pages_total;
//  pages_needed_for_page_stack = kstd::div_ceil(size_of_page_stack, PAGE_SIZE);
//  for (unsigned page = 0; page < pages_needed_for_page_stack; ++page) {
//    paging::kernel_page_tables.identity_map_page_into_kernel_space(
//        (uintptr_t)base_of_page_stack + page * PAGE_SIZE);
//  }
//  const auto physical_base_of_page_stack = (uintptr_t)base_of_page_stack;
//  base_of_page_stack =
//      paging::identity_map_address_to_kernel_space(base_of_page_stack);
//  top_of_page_stack =
//      paging::identity_map_address_to_kernel_space(top_of_page_stack);
//
//  // Populate the page stack with the rest of the available pages
//  for (unsigned long long page = max_pages_total - 1;
//       page >= pages_needed_for_page_stack; --page) {
//    *top_of_page_stack++ = physical_base_of_page_stack + page * PAGE_SIZE;
//  }
//
//  // Adjust top of page stack, since we overshot by one in the last iteration
//  --top_of_page_stack;
//
//  printf("pma : initialized %lu page(s) at 0x%p\n", max_pages_total,
//         base_of_page_stack);
//}
