#include "vma.h"

#include "debug.h"
#include "memory.h"
#include "paging.h"
#include "pma.h"
#include "util.h"
#include "panic.h"

#include <assert.h>

using memory::PAGE_SIZE;
using memory::PAGE_ALIGN;

constexpr static auto ACTIVE_BITS_IN_VIRTUAL_ADDR = 42;
constexpr static uintptr_t MIN_POSITIVE_VIRTUAL_ADDR = 0;
constexpr static uintptr_t MAX_POSITIVE_VIRTUAL_ADDR =
    ((uintptr_t)1 << ACTIVE_BITS_IN_VIRTUAL_ADDR) - 1;
constexpr static uintptr_t MIN_NEGATIVE_VIRTUAL_ADDR =
    -((uintptr_t)1 << ACTIVE_BITS_IN_VIRTUAL_ADDR);
constexpr static uintptr_t MAX_NEGATIVE_VIRTUAL_ADDR = (uintptr_t)-1;

namespace vma {
struct free_node {
  void *base;
  size_t size;
  free_node *next;
};

free_node *virtual_free_list = nullptr;
size_t free_list_size = 0;
size_t max_free_list_size = 0;
constexpr auto NODES_PER_PAGE = PAGE_SIZE / sizeof(free_node);

static void add_node_to_free_list(void *base, size_t size) {
  free_node *node = nullptr;
  free_node *last = nullptr;
  free_node *empty_node = nullptr;

  // Walk free list, looking for nodes to merge or re-use with if possible
  for (node = virtual_free_list, last = node; node != nullptr;
       last = node, node = node->next) {
    // Keep track of empty nodes to re-use in case we can't merge
    if (empty_node == nullptr && node->size == 0) {
      empty_node = node;
      continue;
    }
    const auto end_of_current_node =
        reinterpret_cast<uintptr_t>(node->base) + node->size - 1;
    const auto end_of_new_node = reinterpret_cast<uintptr_t>(base) + size - 1;
    // If the area to be freed is just before an existing free node,
    // move the base of the node to the base of the free area and
    // increase the node's size accordingly
    if ((uintptr_t)node->base - 1 == end_of_new_node) {
      node->base = base;
      node->size += size;
      return;
    }
    // If the area to be freed is at the end of an existing free node,
    // just increase the size of the free node
    else if ((uintptr_t)base - 1 == end_of_current_node) {
      node->size += size;
      return;
    }
  }

  // Otherwise, there are no nodes to merge with. If we have an empty node to
  // re-use, do so now, else create a new node after the last to store the free
  // area.
  if (empty_node == nullptr) {
    empty_node = last + 1;
    last->next = empty_node;

    // TODO: grow the free list when it hits max_free_list_size
    ++free_list_size;
    assert(free_list_size != max_free_list_size && "No more free nodes!");
  }
  *empty_node = free_node{base, size, nullptr};
}

void remove_from_free_list(void *base, size_t size) {
  assert(virtual_free_list && "free list is empty?");
  size = kstd::align_to(size, PAGE_ALIGN);
  for (auto node = virtual_free_list; node != nullptr; node = node->next) {
    const auto end_of_node =
        reinterpret_cast<char *>(node->base) + node->size - 1;
    const auto end_of_area = reinterpret_cast<char *>(base) + size - 1;
    if (base >= node->base && end_of_area <= end_of_node) {
      // Split node
      const auto remaining_front_size = reinterpret_cast<uintptr_t>(base) -
                                        reinterpret_cast<uintptr_t>(node->base);
      const auto remaining_end_size = end_of_node - end_of_area;
      node->size = remaining_front_size;
      add_node_to_free_list(end_of_area + 1, remaining_end_size);
      return;
    }
  }
  kstd::panic("couldn't find region from %p to %p in free list: was it "
              "already freed?",
              base, (char *)base + size);
}

void init() {
  // Identity map a page for the virtual free list to reside in
  virtual_free_list = reinterpret_cast<free_node *>(pma::get_physical_page());
  virtual_free_list =
      paging::kernel_page_tables.identity_map_page_into_kernel_space(
          virtual_free_list);

  max_free_list_size = NODES_PER_PAGE;

  // Initial null entry
  virtual_free_list->base = 0;
  virtual_free_list->size = 0;
  virtual_free_list->next = nullptr;

  const auto mark_free_range = [](intptr_t start, intptr_t end) {
    if (start < end)
      add_node_to_free_list((void *)start, (size_t)(end - start + 1));
    else
      add_node_to_free_list((void *)end, (size_t)(start - end + 1));
  };

  // FFFF'FFFF'8010'0000 - FFFF'FFFF'8020'0000 is mapped for kernel
  // 0000'0000'0000'0000 - 0000'0000'0020'0000 is identity mapped
  //
  // Hence 0000'0000'0020'0000 - FFFF'FFFF'8010'0000 is free,
  //   and FFFF'FFFF'8020'0000 - FFFF'FFFF'FFFF'FFFF is free.
  //
  mark_free_range(0x200000, MAX_POSITIVE_VIRTUAL_ADDR);
  mark_free_range(MIN_NEGATIVE_VIRTUAL_ADDR, memory::KERNEL_VMA_START - 1);
  mark_free_range(memory::KERNEL_VMA_END, MAX_NEGATIVE_VIRTUAL_ADDR);

  // Remove the identity page used by the virtual free list
  remove_from_free_list(virtual_free_list, PAGE_SIZE);
  puts("vma: initialized");
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

void dump_free_list() {
  fprintf(stderr, "dumping VMA free list:\n");
  for (free_node *node = virtual_free_list; node != nullptr;
       node = node->next) {
    fprintf(stderr, "-- node: %p - %p (%zx)\n", node->base,
            (char *)node->base + node->size - 1, node->size);
  }
}
} // namespace vma
