#include "memory.h"

#include <stddef.h>
#include <string.h>
#include "util.h"

static void sort_memory_map();
static void init_physical_memory_allocator();
static void init_virtual_memory_allocator();

void init_memory() {
   sort_memory_map();
   init_physical_memory_allocator();
   init_virtual_memory_allocator();
}

const auto & g_num_of_memory_map_entries = * reinterpret_cast<uint32_t *>            (0x1000);
      auto & g_memory_map                = * reinterpret_cast<memory_map_entry (*)[]>(0x1008);

void sort_memory_map() {
   // Sort the physical memory map returned by the bootloader, putting the largest usable regions
   // first in our list
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
   // We can't actually use 0x0 - 0x500 because this memory is owned by the BIOS,
   // so adjust the region that claims it so we don't use it.
   kstd::transform(g_memory_map, g_num_of_memory_map_entries,
      +[](memory_map_entry & entry) {
         if (entry.base < (char *) 0x500) {
            entry.base = (char * ) 0x500;
            entry.length -= 0x500 - (uint64_t) entry.base;
            return true;
         }
         return false;
   });
}

char      * physical_memory_base = nullptr;
uint64_t (* page_bitmap)[]       = nullptr;
constexpr auto SIZE_OF_BITMAP_ENTRY   = sizeof(decltype(**page_bitmap));
constexpr auto PAGES_PER_BITMAP_ENTRY = SIZE_OF_BITMAP_ENTRY * 8;
auto page_bitmap_size = 0;

// 4KiB pages
constexpr static int PAGE_SIZE = 0x1000;

void init_physical_memory_allocator() {
   const auto & largest_usable_memory_map_entry = g_memory_map[0];
   // Page align physical_base at bottom of largest memory region
   physical_memory_base = reinterpret_cast<char*>((uintptr_t)largest_usable_memory_map_entry.base
                                                             & (~PAGE_SIZE));

   page_bitmap = reinterpret_cast<decltype(page_bitmap)>(&__KERNEL_END__);

   const auto number_of_pages = largest_usable_memory_map_entry.length / PAGE_SIZE;
   page_bitmap_size = (number_of_pages / PAGES_PER_BITMAP_ENTRY) * SIZE_OF_BITMAP_ENTRY;
   memset(page_bitmap, 0, page_bitmap_size);

   // Mark the first block of pages as used.
   // These are given to the kernel.
   (*page_bitmap)[0] = ~0;
}

void * get_physical_pages(size_t n) {
   for (auto block_number = 0; block_number < page_bitmap_size; ++block_number) {
      auto& page_block = (*page_bitmap)[block_number];
      if (page_block != (uint64_t) ~0) {
         // Found a block with a free page
         for (unsigned long long offset = 0; offset < PAGES_PER_BITMAP_ENTRY; ++offset) {
            const auto bit = 1 << offset;
            if ((page_block & bit) ^ bit) {
               // Found the offset of a free page
               // Mark it used and return actual page base address
               page_block ^= bit;
               const auto page_number  = block_number * PAGES_PER_BITMAP_ENTRY + offset;
               const auto page_address = physical_memory_base + page_number * PAGE_SIZE;
               return page_address;
            }
         }
      }
   }
   // Allocation failed
   return nullptr;
}

void * get_physical_page() { return get_physical_pages(1); }

char * vbase = nullptr;

page_map_level_4_table page_table_base;

void init_virtual_memory_allocator() {
   vbase = 0;
   uintptr_t base = 0;
   asm("mov %cr3, %rax");
   asm("mov %%rax, %0" : "=g"(base));
   page_table_base = reinterpret_cast<decltype(page_table_base)>(base);
   vbase += PAGE_SIZE;
}

void * get_virtual_pages(size_t n) {
   auto virtual_block_start = vbase;
   vbase += n * PAGE_SIZE;
   return virtual_block_start;
}

// Each prefix is 9 bits wide (1 << 9 = 0x1FF)
// Each page (and consequently, each page table) must be aligned to 4KiB (1 << 12);
constexpr unsigned long long PREFIX_MASK                 = 0x1FF;
constexpr unsigned long long PAGE_TABLE_MASK             = PREFIX_MASK                 << 12;
constexpr unsigned long long PAGE_DIRECTORY_MASK         = PAGE_TABLE_MASK             << 9;
constexpr unsigned long long PAGE_DIRECTORY_POINTER_MASK = PAGE_DIRECTORY_MASK         << 9;
constexpr unsigned long long PAGE_MAP_LEVEL_4_MASK       = PAGE_DIRECTORY_POINTER_MASK << 9;

void * kmmap(size_t n) {
   size_t pages_to_map = (n / PAGE_SIZE) + 1;
   // We need to hit the page tables in this order
   // page_map_level_4 -> page_directory_pointer_table -> page_directory -> page_table
   // For each level, we need a block of memory 4KiB (0x1000 bytes) wide, since each table has 512 (0x200) entries that are 8 bytes wide.
   // A page itself is 4KiB, so for each level that is null, we allocate a physical page and insert the level there.
   void * physical_page = get_physical_pages(pages_to_map);
   void * virtual_page  = get_virtual_pages(pages_to_map);

   auto virtual_page_address = reinterpret_cast<uintptr_t>(virtual_page);

   const auto page_table_offset             = (virtual_page_address & PAGE_TABLE_MASK            ) >> 12;
   const auto page_directory_offset         = (virtual_page_address & PAGE_DIRECTORY_MASK        ) >> 21;
   const auto page_directory_pointer_offset = (virtual_page_address & PAGE_DIRECTORY_POINTER_MASK) >> 30;
   const auto page_map_level_4_offset       = (virtual_page_address & PAGE_MAP_LEVEL_4_MASK      ) >> 39;

   const auto allocate_level_if_null = [](auto& entry) {
      if (reinterpret_cast<char *>(entry) == nullptr) {
         auto new_page_address = reinterpret_cast<uintptr_t>(get_physical_page()) | PAGE_WRITE | PAGE_PRESENT;
         entry = reinterpret_cast<decltype(entry)>(new_page_address);
         auto entry_ptr = reinterpret_cast<char *>(entry);
         memset_v(&entry_ptr[0], 0, PAGE_LEVEL_SIZE);
      }
      return entry;
   };

   auto pdpt = allocate_level_if_null(page_table_base[page_map_level_4_offset]);
   auto pdpt_ptr = reinterpret_cast<page_directory_pointer_table *>(pdpt & ~PREFIX_MASK);
   auto pdt  = allocate_level_if_null((*pdpt_ptr)[page_directory_pointer_offset]);
   auto pdt_ptr = reinterpret_cast<page_directory_table *>(pdt & ~PREFIX_MASK);
   auto pt   = allocate_level_if_null((*pdt_ptr)[page_directory_offset]);
   auto pt_ptr = reinterpret_cast<page_table *>(pt & ~PREFIX_MASK);
   auto & page_entry = (*pt_ptr)[page_table_offset];

   page_entry = reinterpret_cast<uintptr_t>(physical_page) | PAGE_WRITE | PAGE_PRESENT;

   return virtual_page;
}

void kunmap(void * p) {

}

void * kmalloc(size_t n) { return kmmap(n); }
void kfree(void * p) { return kunmap(p); }
