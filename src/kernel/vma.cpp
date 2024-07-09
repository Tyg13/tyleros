#include "vma.h"

#include "debug.h"
#include "memory.h"
#include "paging.h"
#include "pma.h"
#include "util.h"

#include <assert.h>

using memory::PAGE_SIZE;
using memory::PAGE_ALIGN;
extern char __text_start__;

namespace vma {
struct free_node {
  void *base;
  size_t size;
  free_node *next;
};

free_node *virtual_free_list;
size_t free_list_size;
size_t max_free_list_size;
constexpr auto NODES_PER_PAGE = PAGE_SIZE / sizeof(free_node);

static void add_node_to_free_list(void *base, size_t size);

void init() {
  // Identity map a page for the virtual free list to reside in
  virtual_free_list = reinterpret_cast<free_node *>(pma::get_physical_page());
  paging::kernel_page_tables.identity_map_page((uintptr_t)virtual_free_list);

  max_free_list_size = NODES_PER_PAGE;

  // Initial null entry
  virtual_free_list->base = 0;
  virtual_free_list->size = 0;
  virtual_free_list->next = nullptr;

  // 0x0 - 0x200000 is identity mapped
  // KERNEL_START - KERNEL_END is the kernel
  //
  // Hence 0x200000 - KERNEL_START is free,
  // and KERNEL_END - UINTPTR_MAX is free
  const static uintptr_t KERNEL_START = (uintptr_t)&__text_start__;
  const static uintptr_t KERNEL_END = KERNEL_START + 0x200000;
  add_node_to_free_list(reinterpret_cast<void *>(0x200000),
                        KERNEL_START - 0x200000);
  add_node_to_free_list(reinterpret_cast<void *>(KERNEL_END),
                        UINTPTR_MAX - KERNEL_END);

  const auto remove_from_free_list = [](void *base, size_t size) {
    size = kstd::align_to(size, PAGE_ALIGN);
    for (auto node = virtual_free_list; node != nullptr; node = node->next) {
      const auto end_of_node =
          reinterpret_cast<char *>(node->base) + node->size;
      const auto end_of_area = reinterpret_cast<char *>(base) + size;
      if (base >= node->base && end_of_area <= end_of_node) {
        // Split node
        const auto remaining_front_size =
            reinterpret_cast<uintptr_t>(base) -
            reinterpret_cast<uintptr_t>(node->base);
        const auto remaining_end_size = end_of_node - end_of_area;
        node->size = remaining_front_size;
        add_node_to_free_list(end_of_area, remaining_end_size);
        return;
      }
    }
  };

  // Remove the identity pages used by the page stack and the
  // virtual free list from the free list
  remove_from_free_list(pma::get_base_of_page_stack(),
                        pma::get_size_of_page_stack());
  remove_from_free_list(virtual_free_list, PAGE_SIZE);
  debug::puts("vma: initialized");
}

static void add_node_to_free_list(void *base, size_t size) {
  free_node *node = nullptr;
  free_node *last = nullptr;
  free_node *empty_node = nullptr;

  // Walk free list, looking for nodes to merge with if possible
  for (node = virtual_free_list, last = node; node != nullptr;
       last = node, node = node->next) {
    const auto end_of_current_node =
        reinterpret_cast<uintptr_t>(node->base) + node->size;
    const auto end_of_new_node = reinterpret_cast<uintptr_t>(base) + size;
    // If the area to be freed is just before an existing free node,
    // move the base of the node to the base of the free area and
    // increase the node's size accordingly
    if (node->base == reinterpret_cast<void *>(end_of_new_node)) {
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
    // Keep track of empty nodes in case we can't merge
    if (empty_node == nullptr && node->size == 0) {
      empty_node = node;
    }
  }

  // Otherwise, there are no nodes to merge with.
  // Use an existing empty node, or if one doesn't exist,
  // create a new node after the last to store the free area.
  if (empty_node == nullptr) {
    empty_node = last + 1;
    last->next = empty_node;
  }
  *empty_node = free_node{base, size, nullptr};

  // TODO: grow the free list when it hits max_free_list_size
  ++free_list_size;

  assert(free_list_size != max_free_list_size && "No more free nodes!");
}

void *get_virtual_pages(size_t size) {
  assert(size != 0 && "Tried to allocate page of size 0!");
  assert(size % PAGE_SIZE == 0 && "size must be page-aligned!");

  // Walk the free list for a node of appropriate size.
  for (free_node *node = virtual_free_list; node != nullptr;
       node = node->next) {
    // Found a node big enough
    if (node->size >= size) {
      // Adjust the remaining size in the node
      const auto free_page_address = node->base;
      const auto end_of_requested_area =
          reinterpret_cast<uintptr_t>(node->base) + size;
      node->base = reinterpret_cast<void *>(end_of_requested_area);
      node->size -= size;
      return free_page_address;
    }
  }
  kstd::panic("Allocation failed, out of virtual pages!");
}

void free_virtual_pages(void *address, size_t size) {
  assert(address != nullptr && "Tried to free page at 0x0!");
  assert(size != 0 && "Tried to free page of size 0!");
  add_node_to_free_list(address, size);
}
} // namespace vma
