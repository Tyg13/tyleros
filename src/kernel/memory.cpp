#include "memory.h"

#include <stddef.h>

#include "util.h"

static void sort_memory_map();
static void init_physical_memory_allocator();

void init_memory() {
   sort_memory_map();
   init_physical_memory_allocator();
}

const auto & g_num_of_memory_map_entries = * reinterpret_cast<uint32_t *>            (0x1000);
      auto & g_memory_map                = * reinterpret_cast<memory_map_entry (*)[]>(0x1008);

void sort_memory_map() {
   kstd::insertion_sort(g_memory_map, g_num_of_memory_map_entries,
      +[](const memory_map_entry & lhs, const memory_map_entry & rhs) {
         const auto entry_is_usable =
            [](const auto & entry) { return entry.type == memory_map_entry::type::usable; };
         if (!entry_is_usable(lhs)) {
            // If the lhs is unusable, the rhs is better.
            return false;
         }
         if (!entry_is_usable(rhs)) {
            // If the rhs is unusable, the lhs is better.
            return true;
         }
         // If both are usable regions, the larger one is better.
         return lhs.length > rhs.length;
   });
   kstd::transform(g_memory_map, g_num_of_memory_map_entries,
      +[](memory_map_entry & entry) {
         if (entry.base == 0x0) {
            entry.base += 0x500;
            entry.length -= 0x500;
            return true;
         }
         return false;
   });
}

char  * pbase          = nullptr;
uint64_t (* page_bitmap)[] = nullptr;
constexpr auto PAGES_PER_BITMAP_ENTRY = sizeof(decltype(**page_bitmap)) * 8;
constexpr auto SIZE_OF_BITMAP_ENTRY   = sizeof(decltype(**page_bitmap));
auto page_bitmap_size = 0;

// 4KiB pages
constexpr static int PAGE_SIZE = 0x400;

void init_physical_memory_allocator() {
   const auto & largest_usable_memory_map_entry = g_memory_map[0];
   // Page align pbase at bottom of largest memory region
   pbase       = reinterpret_cast<char*>((uintptr_t)largest_usable_memory_map_entry.base
                                                    & (~PAGE_SIZE));
   page_bitmap = reinterpret_cast<decltype(page_bitmap)>(pbase);

   const auto number_of_pages = largest_usable_memory_map_entry.length / PAGE_SIZE;
   page_bitmap_size = (number_of_pages / PAGES_PER_BITMAP_ENTRY) * SIZE_OF_BITMAP_ENTRY;

   // Mark first page as used
   (*page_bitmap)[0] ^= 1;
}

void * get_physical_page() {
   for (auto block_number = 0; block_number < page_bitmap_size; ++block_number) {
      auto& page_block = (*page_bitmap)[block_number];
      if (page_block != (uint64_t) ~0) {
         // Found a block with a free page
         for (auto offset = 7; offset >= 0; ++offset) {
            const auto bit = 1 << offset;
            if (page_block ^ bit) {
               // Found the offset of the free page
               // Mark it used and return actual page base address
               page_block ^= bit;
               const auto page_number = block_number + offset;
               return pbase + page_number * PAGE_SIZE;
            }
         }
      }
   }
   // Allocation failed
   return nullptr;
}
