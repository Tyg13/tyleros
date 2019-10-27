#include "pma.h"

#include "paging.h"
#include "memory.h"
#include "util.h"

#include <stdint.h>

bool physical_memory_allocator_available = false;

uintptr_t physical_memory_base = (uintptr_t) nullptr;
unsigned long long pages_total = 0;

uintptr_t * base_of_page_stack;
uintptr_t * top_of_page_stack;

void init_physical_memory_allocator() {
   const auto & largest_usable_memory_map_entry = get_memory_map()[0];
   // Page align physical_base at bottom of largest memory region
   physical_memory_base = largest_usable_memory_map_entry.base & ~(PAGE_SIZE - 1);

   pages_total = largest_usable_memory_map_entry.length / PAGE_SIZE; 

   base_of_page_stack = reinterpret_cast<uintptr_t *>(physical_memory_base);
   top_of_page_stack  = base_of_page_stack;

   // Identity map pages needed for the page stack itself
   const auto size_of_page_stack = sizeof(uintptr_t) * pages_total;
   const auto pages_needed_for_page_stack = div_round_up(size_of_page_stack, PAGE_SIZE);
   for (unsigned long long i = 0; i < pages_needed_for_page_stack; ++i) {
      const auto page = reinterpret_cast<void *>(physical_memory_base + i * PAGE_SIZE);
      map_page(page, page);
   }


   for (unsigned long long page = 0; page < pages_total; ++page) {
      *top_of_page_stack++ = physical_memory_base + page * PAGE_SIZE;
   }

   physical_memory_allocator_available = true;
}

void * get_physical_page() {
   if (top_of_page_stack > base_of_page_stack) {
      return reinterpret_cast<void *>(*--top_of_page_stack);
   }
   return nullptr;
}

void free_physical_page(void * page) {
   *top_of_page_stack++ = reinterpret_cast<uintptr_t>(page);
}
