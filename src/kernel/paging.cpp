#include "paging.h"

#include "debug.h"
#include "low_memory_allocator.h"
#include "memory.h"
#include "util.h"

#include <assert.h>
#include <string.h>

using namespace memory;

namespace paging {
page_tables kernel_page_tables;
static volatile uintptr_t *find_page_entry(const page_tables &tables,
                                           void *addr);

constexpr static auto PAGE_RW = paging::attributes::RW;
constexpr static auto PAGE_PRESENT = paging::attributes::PRESENT;
constexpr static auto PAGE_XD = paging::attributes::XD;

static void *allocate_page_level() {
  const auto addr = low_memory::allocate(PAGE_SIZE, PAGE_ALIGN);
  // TODO actually grab more page tables if we run out of low memory.
  assert(addr != nullptr && "No more room for page tables!");
  return addr;
}

void early_init() {
  uintptr_t dummy;
  asm volatile("mov %%cr3, %0" : "=&r"(dummy));
  kernel_page_tables.base = (page_level *)dummy;
  kernel_page_tables.allocate_page_level = &allocate_page_level;
}

static uint64_t readmsr(uint32_t n) {
  uint32_t efer_lo, efer_hi;
  asm("rdmsr" : "=d"(efer_hi), "=a"(efer_lo) : "c"(n));
  return (uint64_t)efer_lo | ((uint64_t)efer_hi << 32);
}

static void writesmr(uint32_t n, uint64_t val) {
  uint32_t lo = (uint32_t)val;
  uint32_t hi = (uint32_t)(val >> 32);
  asm("wrmsr" ::"c"(n), "d"(hi), "a"(lo));
}

void finish_init() {
  // Unmap the zero page so accesses through nullptr cause a page fault
  kernel_page_tables.unmap_page(0);
  invlpg(0);

  // Enable XD (execute-disable bit) for pages
  uint64_t efer = readmsr(0xC0000080);
  writesmr(0xC0000080, efer | (1 << 11));

  // Turn on write-protection for kernel (can't write to read-only pages)
  asm volatile("mov %%cr0, %%rax\n"
               "or %%rax, %c0\n"
               "mov %%rax, %%cr0" ::"i"(1 << 16)
               : "rax", "memory");

  // Set up appropriate page permissions for the various segments linked into
  // the kernel
  debug::printf(
      "Paging init: .text : 0x%p - 0x%p (0x%lx - 0x%lx) = EXEC | READ\n",
      &__text_start__, &__text_end__,
      kernel_page_tables.get_physical_address(&__text_start__),
      kernel_page_tables.get_physical_address(&__text_end__));
  for (char *c = &__text_start__; c != &__text_end__; c += PAGE_SIZE) {
    volatile uintptr_t *entry = find_page_entry(kernel_page_tables, c);
    assert(entry && "no text page mapped?");
    assert(*entry & PAGE_PRESENT && "text page not present?");
    *entry = *entry & ~(PAGE_XD | PAGE_RW);
  }

  debug::printf("Paging init: .rodata : 0x%p - 0x%p (0x%lx - 0x%lx) = READ\n",
                &__rodata_start__, &__rodata_end__,
                kernel_page_tables.get_physical_address(&__rodata_start__),
                kernel_page_tables.get_physical_address(&__rodata_end__));
  for (char *c = &__rodata_start__; c != &__rodata_end__; c += PAGE_SIZE) {
    volatile uintptr_t *entry = find_page_entry(kernel_page_tables, c);
    assert(entry && "no rodata page mapped?");
    assert(*entry & PAGE_PRESENT && "rodata page not present?");
    *entry = (*entry | PAGE_XD) & ~PAGE_RW;
  }

  debug::printf(
      "Paging init: .data : 0x%p - 0x%p (0x%lx - 0x%lx) = READ | WRITE\n",
      &__data_start__, &__data_end__, kernel_page_tables.get_physical_address(&__data_start__),
      kernel_page_tables.get_physical_address(&__data_end__));
  for (char *c = &__data_start__; c != &__data_end__; c += PAGE_SIZE) {
    volatile uintptr_t *entry = find_page_entry(kernel_page_tables, c);
    assert(entry && "no data page mapped?");
    assert(*entry & PAGE_PRESENT && "data page not present?");
    *entry = *entry | PAGE_XD | PAGE_RW;
  }

  debug::printf(
      "Paging init: .bss : 0x%p - 0x%p (0x%lx - 0x%lx) = READ | WRITE\n",
      &__bss_start__, &__bss_end__,
      kernel_page_tables.get_physical_address(&__bss_start__),
      kernel_page_tables.get_physical_address(&__bss_end__));
  for (char *c = &__bss_start__; c != &__bss_end__; c += PAGE_SIZE) {
    volatile uintptr_t *entry = find_page_entry(kernel_page_tables, c);
    assert(entry && "no bss page mapped?");
    assert(*entry & PAGE_PRESENT && "bss page not present?");
    *entry = *entry | PAGE_XD | PAGE_RW;
  }

  debug::puts("Paging init: all kernel pages now protected");
}

// Each prefix is 9 bits wide.
// Each page (and consequently, each page table) must be aligned to 4KiB (1 <<
// 12);
constexpr uintptr_t PREFIX_MASK = (1 << 9) - 1;
constexpr uintptr_t PAGE_TABLE_MASK = PREFIX_MASK << 12;
constexpr uintptr_t PAGE_DIRECTORY_MASK = PAGE_TABLE_MASK << 9;
constexpr uintptr_t PAGE_DIRECTORY_POINTER_MASK = PAGE_DIRECTORY_MASK << 9;
constexpr uintptr_t PAGE_MAP_LEVEL_4_MASK = PAGE_DIRECTORY_POINTER_MASK << 9;
constexpr uintptr_t ADDRESS_MASK = 0x000FFFFFFFFFF000;

static volatile uintptr_t *get_page_entry(page_tables &tables,
                                          uintptr_t virtual_page_address,
                                          bool allocate_levels_if_null) {
  // We need to hit the page tables in the order
  // page_map_level_4 -> page_directory_pointer_table -> page_directory ->
  // page_table
  //
  // For each level, we need a block of memory 4KiB (0x1000 bytes) wide, since
  // each table has 512 (0x200) entries that are 8 bytes wide.
  //
  // A page itself is 4KiB, so for each level that is null, we allocate an
  // identity page and insert the level there.

  const auto page_table_offset = (virtual_page_address & PAGE_TABLE_MASK) >> 12;
  const auto page_directory_offset =
      (virtual_page_address & PAGE_DIRECTORY_MASK) >> 21;
  const auto page_directory_pointer_offset =
      (virtual_page_address & PAGE_DIRECTORY_POINTER_MASK) >> 30;
  const auto page_map_level_4_offset =
      (virtual_page_address & PAGE_MAP_LEVEL_4_MASK) >> 39;

  const auto allocate_level_if_null = [&tables](volatile uintptr_t &
                                                entry) -> volatile uintptr_t & {
    if (reinterpret_cast<void *>(entry) == nullptr) {
      entry = reinterpret_cast<uintptr_t>(tables.allocate_page_level()) |
              PAGE_RW | PAGE_PRESENT;
    }
    return entry;
  };

  page_level *pt_ptr = nullptr;
  if (allocate_levels_if_null) {
    auto pdpt =
        allocate_level_if_null((*tables.base)[page_map_level_4_offset]);
    auto pdpt_ptr = reinterpret_cast<page_level *>(pdpt & ~PREFIX_MASK);

    auto pdt = allocate_level_if_null((*pdpt_ptr)[page_directory_pointer_offset]);
    auto pdt_ptr = reinterpret_cast<page_level *>(pdt & ~PREFIX_MASK);

    auto pt = allocate_level_if_null((*pdt_ptr)[page_directory_offset]);
    pt_ptr = reinterpret_cast<page_level *>(pt & ~PREFIX_MASK);
  } else {
    auto pdpt = (*kernel_page_tables.base)[page_map_level_4_offset];
    if (auto pdpt_ptr = reinterpret_cast<page_level *>(pdpt & ~PREFIX_MASK)) {
      auto pdt = (*pdpt_ptr)[page_directory_pointer_offset];
      if (auto pdt_ptr = reinterpret_cast<page_level *>(pdt & ~PREFIX_MASK)) {
        auto pt = (*pdt_ptr)[page_directory_offset];
        pt_ptr = reinterpret_cast<page_level *>(pt & ~PREFIX_MASK);
      }
    }
  }

  return pt_ptr ? &(*pt_ptr)[page_table_offset] : nullptr;
}

static volatile uintptr_t *allocate_page_entry(page_tables &tables,
                                               uintptr_t addr) {
  return get_page_entry(tables, addr, true);
}
static volatile uintptr_t *find_page_entry(const page_tables &tables,
                                           uintptr_t addr) {
  return get_page_entry(const_cast<page_tables &>(tables), addr, false);
}
static volatile uintptr_t *find_page_entry(const page_tables &tables,
                                           void *addr) {
  return get_page_entry(const_cast<page_tables &>(tables), (uintptr_t)addr,
                        false);
}

uintptr_t page_tables::get_physical_address(void *virtual_addr) const {
  const auto page_addr = (uintptr_t)virtual_addr & -PAGE_SIZE;
  const auto offset_in_page = (uintptr_t)virtual_addr - page_addr;
  const auto *entry = find_page_entry(*this, page_addr);
  assert(entry && "entry not found!");
  return entry ? (*entry & ADDRESS_MASK) + offset_in_page : (uintptr_t) nullptr;
}

void *page_tables::map_range(uintptr_t physical_start, uintptr_t physical_end,
                           uintptr_t virtual_start, uintptr_t virtual_end) {
  assert(virtual_start % memory::PAGE_SIZE == 0 &&
         virtual_end % memory::PAGE_SIZE == 0 &&
         "virtual addresses must be page-aligned!");
  assert(physical_start % memory::PAGE_SIZE == 0 &&
         physical_end % memory::PAGE_SIZE == 0 &&
         "physical addresses must be page-aligned!");
  assert((physical_end - physical_start) == (virtual_end - virtual_start) &&
         "physical and virtual address range sizes must match!");
  for (auto p = physical_start, v = virtual_start;
       p < physical_end && v < virtual_end; p += PAGE_SIZE, v += PAGE_SIZE) {
    map_page(p, v);
  }
  return (void*)virtual_start;
}

void *page_tables::map_range_size(uintptr_t physical_start,
                                uintptr_t virtual_start, size_t size) {
  // Align size to a multiple of a page
  size = kstd::align_to(size, PAGE_ALIGN);
  return map_range(physical_start, physical_start + size, virtual_start,
                   virtual_start + size);
}

void *page_tables::map_page(uintptr_t physical_page, uintptr_t virtual_page,
                          attributes attrs) {
  assert(virtual_page % memory::PAGE_SIZE == 0 &&
         "virtual address must be page-aligned!");
  assert(physical_page % memory::PAGE_SIZE == 0 &&
         "physical address must be page-aligned!");
  auto *page_entry = allocate_page_entry(*this, virtual_page);

  if (!page_entry || (*page_entry & PAGE_PRESENT))
    kstd::panic("Tried to map already-present page!\n"
                "Virtual :%p\n"
                "Physical:%p\n"
                "Entry   :%lx\n",
                (void *)virtual_page, (void *)physical_page, *page_entry);

  *page_entry = physical_page | attrs | attributes::PRESENT;

  return (void*) virtual_page;
}

void page_tables::unmap_page(uintptr_t virtual_page) {
  assert(virtual_page % memory::PAGE_SIZE == 0 &&
         "virtual address should be page-aligned!");
  auto *page_entry = get_page_entry(*this, virtual_page, false);

  if (!page_entry || (*page_entry & PAGE_PRESENT) == 0) {
    kstd::panic("Tried to unmap non-present page\n"
                "Virtual:%p",
                (void *)virtual_page);
  }

  *page_entry &= ~PAGE_PRESENT;
}

void *page_tables::identity_map_page(uintptr_t address) {
  return map_page(address, address);
}

void *page_tables::identity_map_range_size(uintptr_t addr, size_t size) {
  return map_range_size(addr, addr, size);
}
void page_tables::dump() const {
  for (uint64_t i = 0; i < NUM_PAGE_TABLE_ENTRIES; ++i) {
    const uintptr_t pml4_entry = (*base)[i];
    if ((pml4_entry & PAGE_PRESENT) == 0)
      continue;

    const page_level *pdpt = (page_level*)(pml4_entry & ~PREFIX_MASK);
    for (uint64_t j = 0; j < NUM_PAGE_TABLE_ENTRIES; ++j) {
      const uintptr_t pdpt_entry = (*pdpt)[j];
      if ((pdpt_entry & PAGE_PRESENT) == 0)
        continue;

      const page_level *pdt = (page_level*)(pdpt_entry & ~PREFIX_MASK);
      for (uint64_t k = 0; k < NUM_PAGE_TABLE_ENTRIES; ++k) {
        const uintptr_t pdt_entry = (*pdt)[k];
        if ((pdt_entry & PAGE_PRESENT) == 0)
          continue;

        const page_level *pt = (page_level *)(pdt_entry & ~PREFIX_MASK);
        for (uint64_t l = 0; l < NUM_PAGE_TABLE_ENTRIES; ++l) {
          const uintptr_t pt_entry = (*pt)[l];
          if ((pt_entry & PAGE_PRESENT) == 0)
            continue;
          const uintptr_t virtual_addr =
              (l << 12) | (k << 21) | (j << 30) | (i << 39);
          const uintptr_t physical_addr = pt_entry & ADDRESS_MASK;
          const char *rw_flags = (pt_entry & PAGE_RW) ? "rw" : "r";
          const char *x_flags = (pt_entry & PAGE_XD) ? "" : "x";
          debug::printf("Virtual 0x%p -> Physical 0x%p (%s%s)\n",
                        (void *)virtual_addr, (void *)physical_addr, rw_flags,
                        x_flags);
        }
      }
    }
  }
}
} // namespace paging
