#include "memory.h"

#include "paging.h"
#include "pma.h"
#include "util.h"
#include "vma.h"

#include <stddef.h>
#include <string.h>

static void sort_memory_map();
static void init_kmalloc();

void init_memory() {
   sort_memory_map();
   init_paging();
   init_physical_memory_allocator();
   init_virtual_memory_allocator();
   init_kmalloc();
}

memory_map & g_memory_map                = * reinterpret_cast<memory_map *>(0x1008);
const auto & g_num_of_memory_map_entries = * reinterpret_cast<uint32_t   *>(0x1000);

memory_map& get_memory_map() { return g_memory_map; }

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
   const auto mark_unusable = [](uintptr_t start, uintptr_t end) {
      const auto adjust_entry  = [=](memory_map_entry & entry) {
         if (start <= entry.base && entry.base < end) {
            entry.length -= end - entry.base;
            entry.base = end;
            return true;
         }
         return false;
      };
      kstd::transform(g_memory_map, g_num_of_memory_map_entries, adjust_entry);
   };
   // We can't actually use 0x0 - 0x500 because this memory is owned by the BIOS.
   mark_unusable(0x0, 0x500);
   // The kernel is located at 0x100000 - 0x300000
   mark_unusable(0x100000, 0x300000);
}

struct allocation {
   size_t       size;
   size_t       physical_page_list_size;
   uintptr_t  * physical_pages_used;
   allocation * next;
};

// TODO grow the allocation list when it hits the page limit
allocation * allocation_list;

void init_kmalloc() {
   allocation_list = reinterpret_cast<allocation *>(get_physical_page());
   map_page(allocation_list, allocation_list);

   allocation_list->size                    = 0;
   allocation_list->physical_page_list_size = 0;
   allocation_list->physical_pages_used     = nullptr;
   allocation_list->next                    = nullptr;
}

allocation * get_new_allocation() {
   allocation * last = allocation_list;
   while (last->next) {
      last = last->next;
   }
   const auto end_of_last_allocation = reinterpret_cast<uintptr_t *>(last + 1);
   const auto end_of_last_allocation_page_list = end_of_last_allocation + last->physical_page_list_size;
   auto new_allocation = reinterpret_cast<allocation *>(end_of_last_allocation_page_list);
   new_allocation->next = nullptr;

   last->next = new_allocation;

   return new_allocation;
}

void remove_allocation(allocation * node) {
   // Unlink the allocation from the list
   auto * last = allocation_list;
   while (last->next) {
      if (last->next == node) {
         break;
      }
      last = last->next;
   }
   last->next = nullptr;
}

void * kmalloc(size_t n) {
   const auto virtual_address = get_virtual_pages(n);

   auto allocation = get_new_allocation();
   allocation->size = n;
   allocation->physical_pages_used = reinterpret_cast<uintptr_t *>(allocation + 1);

   auto pages_needed = div_round_up(n, PAGE_SIZE);
   for (size_t page = 0; page < pages_needed; ++page) {
      const auto page_offset  = page * PAGE_SIZE;
      const auto virtual_page_address = reinterpret_cast<uintptr_t>(virtual_address) + page_offset;
      const auto virtual_page  = reinterpret_cast<void *>(virtual_page_address);
      const auto physical_page = get_physical_page();
      map_page(physical_page, virtual_page);

      ++allocation->physical_page_list_size;
      allocation->physical_pages_used[page] = reinterpret_cast<uintptr_t>(physical_page);
   }

   auto header = reinterpret_cast<struct allocation **>(virtual_address);
   *header = allocation;

   const auto address_after_header = reinterpret_cast<uintptr_t>(header) + sizeof(struct allocation *);
   return reinterpret_cast<void *>(address_after_header);
}

void kfree(void * p) {
   const auto header_address = reinterpret_cast<uintptr_t>(p) - sizeof(allocation *);
   const auto header = reinterpret_cast<allocation **>(header_address);
   const auto allocation = *header;
   const auto virtual_address = p;
   free_virtual_pages(virtual_address, allocation->size);
   uintptr_t * entry = allocation->physical_pages_used;
   for (size_t page = 0; page < allocation->physical_page_list_size; ++page) {
      const auto physical_page_address = reinterpret_cast<void *>(*entry++);
      free_physical_page(physical_page_address);

      const auto page_offset  = page * PAGE_SIZE;
      const auto virtual_page_address = reinterpret_cast<uintptr_t>(virtual_address) + page_offset;
      const auto virtual_page  = reinterpret_cast<void *>(virtual_page_address);
      unmap_page(virtual_page);
   }

   // Remove the allocation from the allocation list
   remove_allocation(allocation);
}
