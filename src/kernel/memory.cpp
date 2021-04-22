#include "memory.h"

#include "debug.h"
#include "low_memory_allocator.h"
#include "mutex.h"
#include "paging.h"
#include "pma.h"
#include "util.h"
#include "vma.h"

#include <stddef.h>
#include <string.h>

namespace memory {

static void sort_memory_map();
static void init_alloc();
static void dump_memory_map();

uint32_t     g_num_of_memory_map_entries = 0;
memory_map * g_memory_map                = nullptr;

void init(uint32_t memory_map_base,
          uint32_t num_memory_map_entries,
          uint32_t avail_low_mem_start,
          uint32_t avail_low_mem_end) {
    g_memory_map = reinterpret_cast<memory_map*>(memory_map_base);
    g_num_of_memory_map_entries = num_memory_map_entries;

    sort_memory_map();
    low_memory::init(avail_low_mem_start, avail_low_mem_end);
    init_paging();
    init_physical_memory_allocator();
    init_virtual_memory_allocator();
    init_alloc();
}

memory_map& get_memory_map() {
    return *g_memory_map;
}

void sort_memory_map() {
   // Sort the physical memory map returned by the bootloader, putting the largest usable regions
   // first in our list
   kstd::insertion_sort(*g_memory_map, g_num_of_memory_map_entries,
      [](auto&& lhs, auto&& rhs) {
         constexpr auto entry_is_usable = [](auto && entry) { return entry.type == memory_map_entry::usable; };
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
      }
   );
   const auto mark_unusable = [](auto start, auto end) {
      kstd::transform(*g_memory_map, g_num_of_memory_map_entries,
         [=](auto& entry) {
            if ((uintptr_t)start <= entry.base && entry.base < (uintptr_t)end) {
               entry.length -= end - entry.base;
               entry.base = end;
               return true;
            }
            return false;
         }
      );
   };
   // We can't actually use 0x0 - 0x500 because this memory is owned by the BIOS.
   mark_unusable(0x0, 0x500);

   // The kernel is located from 0x100000 - 0x300000
   mark_unusable(0x100000, 0x300000);

   dump_memory_map();
}

void dump_memory_map()
{
    if (!debug::enabled()) {
        return;
    }
    for (auto i = 0; i < (int)g_num_of_memory_map_entries; ++i) {
        const auto entry = (*g_memory_map)[i];
        const auto entry_type = [&]() {
            switch(entry.type) {
                case memory_map_entry::usable: return "usable";
                case memory_map_entry::reserved: return "reserved";
                case memory_map_entry::reclaimable: return "reclaimable";
                case memory_map_entry::nvs: return "nvs";
                case memory_map_entry::badmemory: return "badmemory";
                default: __builtin_unreachable();
            }
        }();
        debug::printf(
                "memory_map[%d]:\n"
                "\ttype: %s\n"
                "\tbase: %lx\n"
                "\tsize: %lx\n", i, entry_type, entry.base, entry.length);
    }
};

struct allocation {
   size_t       size                    = 0;
   size_t       physical_page_list_size = 0;
   allocation * next                    = nullptr;
   uintptr_t  * physical_pages_used     = nullptr;
};

// TODO grow the allocation list as needed
auto allocation_list = (allocation *) { nullptr };

void init_alloc() {
   allocation_list = static_cast<allocation *>(map_one_page());

   *allocation_list = allocation {};
}

static allocation * get_new_allocation(size_t n) {
   auto list = allocation_list;
   assert(list != nullptr, "Tried to get a new allocation, but allocation list is null");

   auto last = list;
   while (last->next) {
      last = last->next;
   }
   const auto end_of_last_allocation = reinterpret_cast<uintptr_t *>(last + 1);
   const auto end_of_last_allocation_page_list = end_of_last_allocation + last->physical_page_list_size;
   auto new_allocation = reinterpret_cast<allocation *>(end_of_last_allocation_page_list);

   const auto pages_needed = div_round_up(n, PAGE_SIZE);
   memset(new_allocation, 0, sizeof(allocation) + pages_needed * sizeof(uintptr_t));
   new_allocation->next = nullptr;
   new_allocation->size = n;
   new_allocation->physical_page_list_size = pages_needed;

   last->next = new_allocation;

   return new_allocation;
}

static void remove_allocation(allocation * node) {
   // Unlink the allocation from the list
   auto * last = allocation_list;
   assert(last != nullptr, "Tried to remove an allocation, but allocation list is null");
   while (last->next != nullptr) {
      if (last->next == node) {
         break;
      }
      last = last->next;
   }
   last->next = nullptr;
}

mutex malloc_lock;

void * alloc(size_t n) {
   mutex_guard lock(malloc_lock);

   auto size_with_header = n + sizeof(struct allocation *);
   size_with_header = ((size_with_header / PAGE_SIZE) + 1) * PAGE_SIZE;
   const auto virtual_address = get_virtual_pages(size_with_header);

   auto allocation = get_new_allocation(size_with_header);

   const auto pages_needed = allocation->physical_page_list_size;

   for (auto page = 0; page < (int)pages_needed; ++page) {
      const auto page_offset   = page * PAGE_SIZE;
      const auto virtual_page  = reinterpret_cast<uintptr_t>(virtual_address) + page_offset;
      const auto physical_page = reinterpret_cast<uintptr_t>(get_physical_page());
      map_page(physical_page, virtual_page);

      allocation->physical_pages_used[page] = reinterpret_cast<uintptr_t>(physical_page);
   }

   auto header = reinterpret_cast<struct allocation **>(virtual_address);
   *header = allocation;

   const auto address_after_header = reinterpret_cast<uintptr_t>(header) + sizeof(struct allocation *);

   return reinterpret_cast<void *>(address_after_header);
}

void free(void * p) {
   mutex_guard lock(malloc_lock);

   const auto header_address = reinterpret_cast<uintptr_t>(p) - sizeof(allocation *);
   const auto header = reinterpret_cast<allocation **>(header_address);
   const auto allocation = *header;

   // Re-add this virtual address to the free list
   const auto virtual_address = header;
   free_virtual_pages(virtual_address, allocation->size);

   // Add each physical page used to the page stack, and unmap each page entry
   uintptr_t * entry = allocation->physical_pages_used;
   for (auto page = 0; page < (int)allocation->physical_page_list_size; ++page) {
      const auto physical_page_address = reinterpret_cast<void *>(*entry++);
      free_physical_page(physical_page_address);

      const auto page_offset  = page * PAGE_SIZE;
      const auto virtual_page = reinterpret_cast<uintptr_t>(virtual_address) + page_offset;
      unmap_page(virtual_page);
   }

   // Remove the allocation from the allocation list
   remove_allocation(allocation);
}

}
