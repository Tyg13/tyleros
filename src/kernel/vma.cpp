#include "vma.h"

#include "memory.h"
#include "paging.h"
#include "pma.h"
#include "util.h"

struct free_node {
   void      * base;
   size_t      size;
   free_node * next;
};

free_node * virtual_free_list;
size_t free_list_size;
size_t max_free_list_size;
constexpr auto NODES_PER_PAGE = PAGE_SIZE / sizeof(free_node);

static void add_node_to_free_list(void * base, size_t size) {
   auto node = virtual_free_list;
   auto last = node;

   // Walk free list, looking for nodes to merge with if possible
   while (node) {
      const auto end_of_current_node = reinterpret_cast<uintptr_t>(node->base) + node->size;
      const auto end_of_new_node     = reinterpret_cast<uintptr_t>(base) + size;
      // If the area to be freed is just before an existing free node,
      // move the base of the node to the base of the free area and
      // increase the node's size accordingly
      if (node->base == reinterpret_cast<void*>(end_of_new_node)) {
         node->base = base;
         node->size += size;
         return;
      }
      // If the area to be freed is at the end of an existing free node,
      // just increase the size of the free node
      else if (base == reinterpret_cast<void *>(end_of_current_node)) {
         node->size += size;
         return;
      }
      last = node;
      node = node->next;
   }

   // Otherwise, no adjacent nodes exist.
   // Create a new node after the last to store the free area.
   auto new_node = last + 1;
   *new_node = free_node { base, size, nullptr };

   last->next = new_node;

   // TODO: grow the free list when it hits max_free_list_size
   ++free_list_size;
}

void init_virtual_memory_allocator() {
   // Identity map a page for the virtual free list to reside in
   virtual_free_list = reinterpret_cast<decltype(virtual_free_list)>(get_physical_page());
   map_page(virtual_free_list, virtual_free_list);

   max_free_list_size = NODES_PER_PAGE;

   // Initial null entry
   virtual_free_list->base = 0;
   virtual_free_list->size = 0;
   virtual_free_list->next = nullptr;

   // TODO: should walk the page tables to determine which virtual addresses
   // are free, since we identity map pages for the physical frame allocator and
   // the virtual memory allocator

   // 0x0 - 0x200000 is identity mapped
   // 0xC000000 - 0xC200000 is the kernel
   //
   // Hence 0x200000 - 0xC000000 is free,
   // and 0xC200000 - UINTPTR_MAX is free
   add_node_to_free_list(reinterpret_cast<void *>( 0x200000),   0xC000000 -  0x200000);
   add_node_to_free_list(reinterpret_cast<void *>(0xC200000), UINTPTR_MAX - 0xC200000);
}

void * get_virtual_pages(size_t n) {
   assert(n != 0, "Tried to allocate page of size 0!");
   // Keep track of the previous node in case we need to remove a node from the free list.
   free_node * prev = nullptr;

   // Walk the free list for a node of appropriate size.
   for (free_node * node = virtual_free_list; node != nullptr; prev = node, node = node->next) {
      // Found a node big enough
      if (node->size >= n) {
         const auto free_page_address = node->base;
         // If the node isn't the exact size of the request, split it
         const auto remaining_size_in_node = node->size - n;
         if (remaining_size_in_node != 0) {
            const auto end_of_requested_area = reinterpret_cast<uintptr_t>(node->base) + n;
            node->base = reinterpret_cast<void *>(end_of_requested_area);
            node->size = remaining_size_in_node;
         }
         // Otherwise, remove the node entirely from the list
         else {
            if (prev) {
               prev->next = node->next;
            }
            // If no prev pointer, node is the root node.
            // Make the next node the root node
            else {
               virtual_free_list = node->next;
            }
         }
         return free_page_address;
      }
   }
   panic("Allocation failed, out of virtual pages!");
   return nullptr;
}

void free_virtual_pages(void * address, size_t size) {
   assert(address != nullptr, "Tried to free page at 0x0!");
   assert(size    != 0,       "Tried to free page of size 0!");
   add_node_to_free_list(address, size);
}
