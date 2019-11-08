#include "pma.h"

#include "paging.h"
#include "memory.h"
#include "util.h"

static uintptr_t physical_memory_base = (uintptr_t) nullptr;

static uintptr_t * base_of_page_stack = nullptr;
static uintptr_t * top_of_page_stack  = nullptr;

uintptr_t * get_base_of_page_stack() {
   return base_of_page_stack;
}

size_t get_size_of_page_stack() {
   return top_of_page_stack - base_of_page_stack;
}

void init_physical_memory_allocator() {
   const auto & largest_usable_memory_map_entry = get_memory_map()[0];
   // Page align physical_base at bottom of largest memory region
   physical_memory_base = largest_usable_memory_map_entry.base & ~(PAGE_SIZE - 1);

   const auto pages_total = largest_usable_memory_map_entry.length / PAGE_SIZE;

   base_of_page_stack = reinterpret_cast<uintptr_t *>(physical_memory_base);
   top_of_page_stack  = base_of_page_stack;

   // Identity map pages needed for the page stack itself
   const auto size_of_page_stack = sizeof(uintptr_t) * pages_total;
   const auto pages_needed_for_page_stack = div_round_up(size_of_page_stack, PAGE_SIZE);
   for (unsigned long long page = 0; page < pages_needed_for_page_stack; ++page) {
      const auto address = reinterpret_cast<void *>(physical_memory_base + page * PAGE_SIZE);
      map_page(address, address);
   }

   // Populate the page stack with the rest of the available pages
   for (unsigned long long page = pages_needed_for_page_stack; page < pages_total; ++page) {
      *top_of_page_stack++ = physical_memory_base + page * PAGE_SIZE;
   }

   // Adjust page stack pointer, since we overshot by one in the last iteration
   --top_of_page_stack;
}

void * get_physical_page() {
   if (top_of_page_stack > base_of_page_stack) {
      const auto new_page = reinterpret_cast<void *>(*top_of_page_stack--);
      assert(new_page != nullptr, "Top of page stack is null!");
      return new_page;
   }
   panic("Allocation failed, out of physical pages!");
   return nullptr;
}

void free_physical_page(void * page) {
   assert(page != nullptr, "Page being freed is null!");
   *++top_of_page_stack = reinterpret_cast<uintptr_t>(page);
}
