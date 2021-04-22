#include "pma.h"

#include "paging.h"
#include "memory.h"
#include "util.h"

static void init_page_stack();

void init_physical_memory_allocator() {
   init_page_stack();
}

static uintptr_t * top_of_page_stack = nullptr;
static uintptr_t base_of_page_stack = (uintptr_t) nullptr;

uintptr_t * get_base_of_page_stack() {
   return reinterpret_cast<uintptr_t *>(base_of_page_stack);
}

size_t get_size_of_page_stack() {
   return reinterpret_cast<uintptr_t>(top_of_page_stack) - base_of_page_stack;
}

void init_page_stack() {
   using memory::PAGE_SIZE;
   const auto & largest_usable_memory_map_entry = memory::get_memory_map()[0];
   const auto pages_total = largest_usable_memory_map_entry.length / PAGE_SIZE;

   // Page align page stack at bottom of largest available memory region
   base_of_page_stack = round_up_to_multiple(largest_usable_memory_map_entry.base, PAGE_SIZE);
   top_of_page_stack  = reinterpret_cast<uintptr_t *>(base_of_page_stack);

   // Identity map pages needed for the page stack itself
   const auto size_of_page_stack = sizeof(uintptr_t) * pages_total;
   const auto pages_needed_for_page_stack = div_round_up(size_of_page_stack, PAGE_SIZE);
   for (auto page = 0; page < (int)pages_needed_for_page_stack; ++page) {
      const auto address = (uintptr_t)(base_of_page_stack + page * PAGE_SIZE);
      map_page(address, address);
   }

   // Populate the page stack with the rest of the available pages
   for (unsigned long long page = pages_needed_for_page_stack; page < pages_total; ++page) {
      *top_of_page_stack++ = base_of_page_stack + page * PAGE_SIZE;
   }

   // Adjust top of page stack, since we overshot by one in the last iteration
   --top_of_page_stack;
}

void * get_physical_page() {
   if (reinterpret_cast<uintptr_t>(top_of_page_stack) <= base_of_page_stack) {
      panic("Allocation failed, out of physical pages!");
   }

   const auto new_page = reinterpret_cast<void *>(*top_of_page_stack--);
   assert(new_page != nullptr, "Top of page stack is null!");
   return new_page;
}

void free_physical_page(void * page) {
   assert(page != nullptr, "Page being freed is null!");
   *++top_of_page_stack = reinterpret_cast<uintptr_t>(page);
}
