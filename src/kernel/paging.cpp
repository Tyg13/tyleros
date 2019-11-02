#include "paging.h"

#include "memory.h"
#include "util.h"

#include <string.h>

static page_map_level_4_table page_table_base;

void init_paging() {
   uintptr_t base = 0;
   asm("mov %%cr3, %0" : "=g"(base));
   page_table_base = reinterpret_cast<decltype(page_table_base)>(base);
}

// Each prefix is 9 bits wide.
// Each page (and consequently, each page table) must be aligned to 4KiB (1 << 12);
constexpr unsigned long long PREFIX_MASK                 = (1 << 9) - 1;
constexpr unsigned long long PAGE_TABLE_MASK             = PREFIX_MASK                 << 12;
constexpr unsigned long long PAGE_DIRECTORY_MASK         = PAGE_TABLE_MASK             << 9;
constexpr unsigned long long PAGE_DIRECTORY_POINTER_MASK = PAGE_DIRECTORY_MASK         << 9;
constexpr unsigned long long PAGE_MAP_LEVEL_4_MASK       = PAGE_DIRECTORY_POINTER_MASK << 9;
constexpr static auto PAGE_LEVEL_SIZE = 512 * sizeof(uintptr_t);

// Page align to next available page after kernel end
static uintptr_t next_available_page_level = (reinterpret_cast<uintptr_t>(&__KERNEL_LMA_END__) + PAGE_SIZE)
                                           & ~(PAGE_SIZE - 1);

static void * allocate_page_level() {
   auto level_address = next_available_page_level;
   next_available_page_level = next_available_page_level + PAGE_LEVEL_SIZE;
   return reinterpret_cast<void *>(level_address);
}

template<bool allocate_level_if_null>
static auto& get_page_entry(uintptr_t virtual_page_address) {
   const auto page_table_offset             = (virtual_page_address & PAGE_TABLE_MASK            ) >> 12;
   const auto page_directory_offset         = (virtual_page_address & PAGE_DIRECTORY_MASK        ) >> 21;
   const auto page_directory_pointer_offset = (virtual_page_address & PAGE_DIRECTORY_POINTER_MASK) >> 30;
   const auto page_map_level_4_offset       = (virtual_page_address & PAGE_MAP_LEVEL_4_MASK      ) >> 39;

   const auto maybe_allocate_level_if_null = [&](auto& entry) -> decltype(auto) {
      if (allocate_level_if_null) {
         if (reinterpret_cast<char *>(entry) == nullptr) {
            auto new_page_address = allocate_page_level();
            memset_v(new_page_address, 0, PAGE_LEVEL_SIZE);

            entry = reinterpret_cast<uintptr_t>(new_page_address) | PAGE_WRITE | PAGE_PRESENT;
         }
      }
      return entry;
   };

   // We need to hit the page tables in the order
   // page_map_level_4 -> page_directory_pointer_table -> page_directory -> page_table
   // For each level, we need a block of memory 4KiB (0x1000 bytes) wide, since each table has 512 (0x200) entries that are 8 bytes wide.
   // A page itself is 4KiB, so for each level that is null, we allocate a physical page and insert the level there.

   auto pdpt     = maybe_allocate_level_if_null(page_table_base[page_map_level_4_offset]);
   auto pdpt_ptr = reinterpret_cast<page_directory_pointer_table *>(pdpt & ~PREFIX_MASK);

   auto pdt      = maybe_allocate_level_if_null((*pdpt_ptr)[page_directory_pointer_offset]);
   auto pdt_ptr  = reinterpret_cast<page_directory_table *>(pdt & ~PREFIX_MASK);

   auto pt       = maybe_allocate_level_if_null((*pdt_ptr)[page_directory_offset]);
   auto pt_ptr   = reinterpret_cast<page_table *>(pt & ~PREFIX_MASK);

   return (*pt_ptr)[page_table_offset];
}

void map_page(void * physical_page, void * virtual_page) {
   auto virtual_page_address = reinterpret_cast<uintptr_t>(virtual_page);

   auto& page_entry = get_page_entry<true>(virtual_page_address);

   assert(!(page_entry & PAGE_PRESENT),
         "Tried to map already-present page!\n"
         "Virtual :%p\n"
         "Physical:%p", virtual_page, physical_page);

   page_entry = reinterpret_cast<uintptr_t>(physical_page) | PAGE_WRITE | PAGE_PRESENT;
}

void unmap_page(void * virtual_page) {
   auto virtual_page_address = reinterpret_cast<uintptr_t>(virtual_page);

   auto &page_entry = get_page_entry<false>(virtual_page_address);

   assert(page_entry & PAGE_PRESENT,
         "Tried to unmap non-present page\n"
         "Virtual:%p", virtual_page);

   page_entry ^= PAGE_PRESENT;

   asm volatile("invlpg (%0)" :: "b"(virtual_page_address) : "memory");
}
