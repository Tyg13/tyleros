#include "paging.h"

#include "low_memory_allocator.h"
#include "memory.h"
#include "pma.h"
#include "util.h"
#include "vma.h"

#include <string.h>

using namespace memory;

namespace paging {
static page_level *page_table_base = nullptr;

void init() {
  uintptr_t base = 0;
  asm("mov %%cr3, %0" : "=r"(base));
  page_table_base = reinterpret_cast<page_level *>(base);

  // Unmap the zero page so accesses through nullptr cause a page fault
  unmap_page(0);
}

// Each prefix is 9 bits wide.
// Each page (and consequently, each page table) must be aligned to 4KiB (1 <<
// 12);
constexpr unsigned long long PREFIX_MASK = (1 << 9) - 1;
constexpr unsigned long long PAGE_TABLE_MASK = PREFIX_MASK << 12;
constexpr unsigned long long PAGE_DIRECTORY_MASK = PAGE_TABLE_MASK << 9;
constexpr unsigned long long PAGE_DIRECTORY_POINTER_MASK = PAGE_DIRECTORY_MASK
                                                           << 9;
constexpr unsigned long long PAGE_MAP_LEVEL_4_MASK = PAGE_DIRECTORY_POINTER_MASK
                                                     << 9;

static void *allocate_page_level() {
  const auto level = low_memory::allocate(PAGE_SIZE);
  // TODO actually grab more page tables if we run out of low memory.
  assert(level != nullptr, "No more room for page tables!");
  return level;
}

static auto &get_page_entry(uintptr_t virtual_page_address) {
  // We need to hit the page tables in the order
  // page_map_level_4 -> page_directory_pointer_table -> page_directory ->
  // page_table
  //
  // For each level, we need a block of memory 4KiB (0x1000 bytes) wide, since
  // each table has 512 (0x200) entries that are 8 bytes wide.
  //
  // A page itself is 4KiB, so for each level that is null, we allocate an
  // identity page and insert the level there.

  const auto page_table_offset = (virtual_page_address & PAGE_TABLE_MASK) >> 12;
  const auto page_directory_offset =
      (virtual_page_address & PAGE_DIRECTORY_MASK) >> 21;
  const auto page_directory_pointer_offset =
      (virtual_page_address & PAGE_DIRECTORY_POINTER_MASK) >> 30;
  const auto page_map_level_4_offset =
      (virtual_page_address & PAGE_MAP_LEVEL_4_MASK) >> 39;

  constexpr auto allocate_level_if_null = [](auto &entry) -> decltype(auto) {
    if (reinterpret_cast<void *>(entry) == nullptr) {
      auto new_page_address = allocate_page_level();
      memset_v(new_page_address, 0, PAGE_SIZE);

      entry = reinterpret_cast<uintptr_t>(new_page_address) | PAGE_WRITE |
              PAGE_PRESENT;
    }
    return entry;
  };

  auto pdpt =
      allocate_level_if_null((*page_table_base)[page_map_level_4_offset]);
  auto pdpt_ptr = reinterpret_cast<page_level *>(pdpt & ~PREFIX_MASK);

  auto pdt = allocate_level_if_null((*pdpt_ptr)[page_directory_pointer_offset]);
  auto pdt_ptr = reinterpret_cast<page_level *>(pdt & ~PREFIX_MASK);

  auto pt = allocate_level_if_null((*pdt_ptr)[page_directory_offset]);
  auto pt_ptr = reinterpret_cast<page_level *>(pt & ~PREFIX_MASK);

  return (*pt_ptr)[page_table_offset];
}

void map_range(uintptr_t physical_start, uintptr_t physical_end,
               uintptr_t virtual_start, uintptr_t virtual_end) {
  for (auto p = physical_start, v = virtual_start;
       p < physical_end && v < virtual_end; p += PAGE_SIZE, v += PAGE_SIZE) {
    map_page(p, v);
  }
}

void map_range_size(uintptr_t physical_start, uintptr_t virtual_start,
                    size_t size) {
  // Align size to a multiple of a page
  size = round_up_to_multiple(size, PAGE_SIZE);
  return map_range(physical_start, physical_start + size, virtual_start,
                   virtual_start + size);
}

void map_page(uintptr_t physical_page, uintptr_t virtual_page) {
  auto &page_entry = get_page_entry(virtual_page);

  assert(!(page_entry & PAGE_PRESENT),
         "Tried to map already-present page!\n"
         "Virtual :%p\n"
         "Physical:%p\n"
         "Entry   :%lx\n",
         (void *)virtual_page, (void *)physical_page, page_entry);

  page_entry = physical_page | PAGE_WRITE | PAGE_PRESENT;
}

void unmap_page(uintptr_t virtual_page) {
  auto &page_entry = get_page_entry(virtual_page);

  assert(page_entry & PAGE_PRESENT,
         "Tried to unmap non-present page\n"
         "Virtual:%p",
         (void *)virtual_page);

  page_entry = page_entry ^ PAGE_PRESENT;

  invlpg(virtual_page);
}

void identity_map_page(uintptr_t address) {
  return map_page(address, address);
}

void *map_one_page() {
  const auto physical_address =
      reinterpret_cast<uintptr_t>(pma::get_physical_page());
  const auto virtual_address =
      reinterpret_cast<uintptr_t>(vma::get_virtual_pages(PAGE_SIZE));
  map_page(physical_address, virtual_address);
  return reinterpret_cast<void *>(virtual_address);
}
} // namespace paging
