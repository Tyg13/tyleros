#include "alloc.h"

#include "assert.h"
#include "memory.h"
#include "mutex.h"
#include "util.h"
#include "vma.h"
#include "pma.h"
#include "paging.h"

#include "libadt/small_string.h"

using memory::PAGE_SIZE;
using memory::PAGE_ALIGN;

template class adt::string_base<kstd::standard_alloc>;

namespace alloc {

struct allocation {
  allocation *next = nullptr;
  size_t size = 0;
  size_t physical_page_list_size = 0;
  uintptr_t physical_pages_used[];
};

kstd::mutex malloc_lock;

struct allocation_list {
  allocation *first = nullptr;
};
allocation_list allocations = {};

void init() {
  const auto physical_address = pma::get_physical_page();
  const auto virtual_address = vma::get_virtual_pages(PAGE_SIZE);
  allocations.first = static_cast<allocation *>(
      paging::kernel_page_tables.map_page(physical_address, virtual_address));

  *allocations.first = allocation{};
}

static allocation *get_new_allocation(size_t n) {
  allocation *list = allocations.first;
  assert(list != nullptr &&
         "Tried to get a new allocation, but allocation list is null");

  // Find the last allocation node in the list. If we find a suitable killed
  // node in the middle of the list instead, use that.
  const auto num_physical_pages_needed = kstd::div_ceil(n, PAGE_SIZE);
  allocation *last = list;
  uint64_t allocation_list_size = 0;
  allocation *new_allocation = nullptr;
  while (true) {
    allocation_list_size +=
        sizeof(allocation) + sizeof(uintptr_t) * last->physical_page_list_size;

    // Re-use killed node with the same physical page size
    if (last->size == 0 &&
        last->physical_page_list_size == num_physical_pages_needed) {
      last->size = n;
      return last;
    }

    // Exit once we've found the last node
    if (last->next == nullptr)
      break;

    last = last->next;
  }

  // Check if we have space in this page for a new allocation after the last
  // one. If the allocation + physical pages would cross a page boundary,
  // allocate a new page and put the new allocation there.
  size_t size_of_new_allocation =
      sizeof(allocation) + sizeof(uintptr_t) * num_physical_pages_needed;
  if (allocation_list_size / PAGE_SIZE !=
      (allocation_list_size + size_of_new_allocation) / PAGE_SIZE) {
    assert(allocation_list_size + size_of_new_allocation <= PAGE_SIZE &&
           "allocation node won't fit in one page!");
    const auto physical_address = pma::get_physical_page();
    const auto virtual_address = vma::get_virtual_pages(PAGE_SIZE);
    new_allocation = static_cast<allocation *>(
        paging::kernel_page_tables.map_page(physical_address, virtual_address));
  } else {
    void *end_of_last_allocation =
        &last->physical_pages_used[last->physical_page_list_size];
    new_allocation = reinterpret_cast<allocation *>(end_of_last_allocation);
  }

  memset(new_allocation, 0, size_of_new_allocation);
  new_allocation->next = nullptr;
  new_allocation->size = n;
  new_allocation->physical_page_list_size = num_physical_pages_needed;

  last->next = new_allocation;

  return new_allocation;
}

static void remove_allocation(allocation *node) {
  // Set the allocation's size to 0 so it can be re-used later if possible
  auto *last = allocations.first;
  assert(last != nullptr &&
         "Tried to remove an allocation, but allocation list is null");
  for (; last->next != nullptr; last = last->next)
    if (last->next == node)
      break;
  last->size = 0;
}

void *alloc(size_t n, kstd::Align alignment, protection p) {
  if (n == 0) return nullptr;

  const kstd::mutex::guard lock = malloc_lock.lock();
  fprintf(stderr, "allocating %zd bytes with %lx alignment\n", n,
          alignment.val);

  // First find out how much space we need for the allocation header + alignment
  // info + padding to meet the given alignment.
  const auto padded_header_size = kstd::align_to(
      sizeof(struct allocation *) + sizeof(kstd::Align::repr_type), alignment);
  fprintf(stderr, "-- header size: %lx\n", padded_header_size);
  // Now figure out how many pages we'll need to allocate `n` bytes + the
  // required space for the header.
  const auto size_with_header =
      kstd::align_to(n + padded_header_size, PAGE_ALIGN);
  // Allocate the virtual addresses
  const auto virtual_base_address = vma::get_virtual_pages(size_with_header);
  fprintf(stderr, "-- starting addr: %p\n", virtual_base_address);

  // Get a new entry in the allocations list
  auto allocation = get_new_allocation(size_with_header);
  fprintf(stderr, "-- allocation addr: %p\n", allocation);

  // Now we need to allocate physical pages to back the virtual addresses, then
  // map physical pages to virtual pages, and record physical page addresses.
  paging::attributes attrs = paging::attributes::NONE;
  switch (p) {
  case READ_ONLY:
    attrs = attrs | paging::attributes::XD;
  case READ_WRITE:
    attrs = attrs | paging::attributes::RW | paging::attributes::RW;
  case EXEC:
    break;
  }
  const auto pages_needed = allocation->physical_page_list_size;
  for (auto page = 0; page < (int)pages_needed; ++page) {
    const auto page_offset = page * PAGE_SIZE;
    const auto virtual_page =
        (void *)((uintptr_t)(virtual_base_address) + page_offset);
    const auto physical_page = pma::get_physical_page();
    paging::kernel_page_tables.map_page(physical_page, virtual_page, attrs);

    allocation->physical_pages_used[page] =
        reinterpret_cast<uintptr_t>(physical_page);
  }

  // Write the pointer to that entry into the allocation header
  auto header = reinterpret_cast<struct allocation **>(virtual_base_address);
  fprintf(stderr, "-- header addr: %p\n", header);
  *header = allocation;

  const auto address_after_header =
      reinterpret_cast<uintptr_t>(virtual_base_address) + padded_header_size;
  void *addr = (void *)address_after_header;
  *((kstd::Align::repr_type *)addr - 1) = alignment.val;
  fprintf(stderr, "-- final addr: %p\n", addr);
  return addr;
}

void free(void *p) {
  if (!p) return;

  const kstd::mutex::guard lock = malloc_lock.lock();
  fprintf(stderr, "freeing %p\n", p);

  const auto alignment = *((kstd::Align::repr_type *)p - 1);
  fprintf(stderr, " -- alignment: %lx\n", alignment);
  const auto padded_header_size =
      kstd::align_to(sizeof(allocation *) + sizeof(kstd::Align::repr_type),
                     kstd::Align{alignment});
  fprintf(stderr, " -- header size: %lx\n", padded_header_size);
  const auto header_address =
      reinterpret_cast<uintptr_t>(p) - padded_header_size;
  const auto header = reinterpret_cast<allocation **>(header_address);
  fprintf(stderr, " -- header addr: %p\n", header);
  const auto allocation = *header;
  fprintf(stderr, " -- allocation addr: %p\n", header);
  assert(allocation && "bad free: can't find the allocation!");

  // Re-add this virtual address to the free list
  const auto virtual_address = header;
  vma::free_virtual_pages(virtual_address, allocation->size);

  // Add each physical page used to the page stack, and unmap each page entry
  uintptr_t *entry = allocation->physical_pages_used;
  for (auto page = 0; page < (int)allocation->physical_page_list_size; ++page) {
    const auto physical_page_address = reinterpret_cast<void *>(*entry++);
    pma::free_physical_page(physical_page_address);

    const auto page_offset = page * PAGE_SIZE;
    const auto virtual_page =
        (void *)((uintptr_t)(virtual_address) + page_offset);
    paging::kernel_page_tables.unmap_page(virtual_page);
  }

  // Remove the allocation from the allocation list
  remove_allocation(allocation);
}
} // namespace alloc
