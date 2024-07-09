#include "alloc.h"

#include "assert.h"
#include "memory.h"
#include "mutex.h"
#include "util.h"
#include "vma.h"
#include "pma.h"
#include "paging.h"

using memory::PAGE_SIZE;
using memory::PAGE_ALIGN;

#include "adt/small_string.h"
template class kstd::string_base<kstd::standard_alloc>;

namespace alloc {

struct allocation {
  size_t size = 0;
  size_t physical_page_list_size = 0;
  allocation *next = nullptr;
  uintptr_t physical_pages_used[];
};

kstd::mutex malloc_lock;

// TODO grow the allocation list as needed
auto allocation_list = (allocation *){nullptr};

void init() {
  const auto physical_address =
      reinterpret_cast<uintptr_t>(pma::get_physical_page());
  const auto virtual_address =
      reinterpret_cast<uintptr_t>(vma::get_virtual_pages(PAGE_SIZE));
  allocation_list = static_cast<allocation *>(
      paging::kernel_page_tables.map_page(physical_address, virtual_address));

  *allocation_list = allocation{};
}

static allocation *get_new_allocation(size_t n) {
  auto list = allocation_list;
  assert(list != nullptr &&
         "Tried to get a new allocation, but allocation list is null");

  auto last = list;
  while (last->next) {
    last = last->next;
  }
  const auto end_of_last_allocation = reinterpret_cast<uintptr_t *>(last + 1);
  const auto end_of_last_allocation_page_list =
      end_of_last_allocation + last->physical_page_list_size;
  auto new_allocation =
      reinterpret_cast<allocation *>(end_of_last_allocation_page_list);

  const auto pages_needed = kstd::div_ceil(n, PAGE_SIZE);
  memset(new_allocation, 0,
         sizeof(allocation) + pages_needed * sizeof(uintptr_t));
  new_allocation->next = nullptr;
  new_allocation->size = n;
  new_allocation->physical_page_list_size = pages_needed;

  last->next = new_allocation;

  return new_allocation;
}

static void remove_allocation(allocation *node) {
  // Unlink the allocation from the list
  auto *last = allocation_list;
  assert(last != nullptr &&
         "Tried to remove an allocation, but allocation list is null");
  while (last->next != nullptr) {
    if (last->next == node) {
      break;
    }
    last = last->next;
  }
  last->next = nullptr;
}

void *alloc(size_t n, kstd::Align alignment, protection p) {
  const kstd::mutex::guard lock = malloc_lock.lock();

  const auto padded_header_size =
      kstd::align_to(sizeof(struct allocation *), alignment);
  const auto size_with_header =
      kstd::align_to(n + padded_header_size, PAGE_ALIGN);
  const auto virtual_address = vma::get_virtual_pages(size_with_header);

  auto header = reinterpret_cast<allocation **>(virtual_address);
  const auto address_after_header =
      reinterpret_cast<uintptr_t>(header) + padded_header_size;

  auto allocation = get_new_allocation(size_with_header);

  const auto pages_needed = allocation->physical_page_list_size;

  for (auto page = 0; page < (int)pages_needed; ++page) {
    const auto page_offset = page * PAGE_SIZE;
    const auto virtual_page =
        reinterpret_cast<uintptr_t>(virtual_address) + page_offset;
    const auto physical_page =
        reinterpret_cast<uintptr_t>(pma::get_physical_page());
    const auto attrs = p == protection::READ_ONLY ? paging::attributes::XD
                       : p == protection::READ_WRITE
                           ? paging::attributes::RW | paging::attributes::XD
                           : paging::attributes::NONE;
    paging::kernel_page_tables.map_page(physical_page, virtual_page, attrs);

    allocation->physical_pages_used[page] =
        reinterpret_cast<uintptr_t>(physical_page);
  }

  *header = allocation;

  return reinterpret_cast<void *>(address_after_header);
}

void free(void *p, kstd::Align alignment) {
  const kstd::mutex::guard lock = malloc_lock.lock();

  const auto padded_header_size =
      kstd::align_to(sizeof(allocation *), alignment);
  const auto header_address =
      reinterpret_cast<uintptr_t>(p) - padded_header_size;
  const auto header = reinterpret_cast<allocation **>(header_address);
  const auto allocation = *header;
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
        reinterpret_cast<uintptr_t>(virtual_address) + page_offset;
    paging::kernel_page_tables.unmap_page(virtual_page);
  }

  // Remove the allocation from the allocation list
  remove_allocation(allocation);
}
} // namespace alloc
