#include "paging.h"

#include "memory.h"
#include "util.h"
#include "pma.h"
#include "vma.h"
#include "panic.h"

#include <assert.h>
#include <string.h>

using namespace memory;

namespace paging {

page_tables kernel_page_tables;

static void invlpg(uintptr_t page) {
  asm volatile("invlpg (%0)" ::"r"(page) : "memory");
}

// 1 PML4, 1 PML3, 1 PML2, 1 PML1
//  - 4 pages of kernel address space needed
//  - (2MB of kernel address space)
alignas(PAGE_ALIGN.val) page_table kernel_pml4 = {0};
alignas(PAGE_ALIGN.val) page_table kernel_pml3 = {0};
alignas(PAGE_ALIGN.val) page_table kernel_pml2 = {0};
alignas(PAGE_ALIGN.val) page_table kernel_pml1 = {0};
alignas(PAGE_ALIGN.val) page_table identity_pml3 = {0};
alignas(PAGE_ALIGN.val) page_table identity_pml2 = {0};
alignas(PAGE_ALIGN.val) page_table identity_pml1 = {0};

constexpr static auto MAX_STATIC_TABLES = 8;
alignas(PAGE_ALIGN.val) page_table static_tables[MAX_STATIC_TABLES] = {{0}};
uint64_t num_static_tables_left = MAX_STATIC_TABLES;

constexpr static auto MAX_DYNAMIC_TABLES = 512;
page_table *dynamic_tables = nullptr;
uint64_t num_dynamic_tables_left = 0;

static uintptr_t allocate_page_level() {
  if (num_static_tables_left > 0) {
    page_table *next_static_table =
        &static_tables[--num_static_tables_left];
    return (uintptr_t)next_static_table - KERNEL_VMA_OFFSET;
  }
  assert(
      dynamic_tables &&
      "ran out of static tables before dynamic tables could be initialized!");
  if (num_dynamic_tables_left > 0) {
    page_table *next_dynamic_table =
        &dynamic_tables[--num_dynamic_tables_left];
    return (uintptr_t)next_dynamic_table - KERNEL_VMA_OFFSET;
  }
  kstd::panic("no more page tables!");
}

void early_init() {
  // Map 0x0000'0000'0010'0000 - 0x0000'0000'0020'0000 (0x200 pages,
  //  to 0xFFFF'FFFF'8010'0000 - 0xFFFF'FFFF'8020'0000  1M for the kernel)
  kernel_pml4[0x1FF] = ((uintptr_t)&kernel_pml3 - KERNEL_VMA_OFFSET) |
                       attributes::RW | attributes::PRESENT;
  kernel_pml3[0x1FE] = ((uintptr_t)&kernel_pml2 - KERNEL_VMA_OFFSET) |
                       attributes::RW | attributes::PRESENT;
  kernel_pml2[0] = ((uintptr_t)&kernel_pml1 - KERNEL_VMA_OFFSET) |
                   attributes::RW | attributes::PRESENT;
  for (unsigned i = 0x100; i < 0x200; ++i) {
    kernel_pml1[i] =
        ((uintptr_t)i * PAGE_SIZE) | attributes::XD | attributes::PRESENT;
  }

  // Identity map the bottom 2M of memory as read-only
  kernel_pml4[0] = ((uintptr_t)&identity_pml3 - KERNEL_VMA_OFFSET) |
                   attributes::RW | attributes::PRESENT;
  identity_pml3[0] = ((uintptr_t)&identity_pml2 - KERNEL_VMA_OFFSET) |
                     attributes::RW | attributes::PRESENT;
  identity_pml2[0] = ((uintptr_t)&identity_pml1 - KERNEL_VMA_OFFSET) |
                     attributes::RW | attributes::PRESENT;
  // Unmap null page
  identity_pml1[0] = attributes::NONE;
  for (unsigned i = 1; i < 0x200; ++i) {
    identity_pml1[i] =
        ((uintptr_t)i * PAGE_SIZE) | attributes::XD | attributes::PRESENT;
  }

  kernel_page_tables.base = &kernel_pml4;
  kernel_page_tables.allocate = &allocate_page_level;
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

void enable_kernel_page_protection(uintptr_t kernel_stack_base) {
  // Enable XD (execute-disable bit) for pages
  const auto efer = readmsr(0xC0000080);
  writesmr(0xC0000080, efer | (1 << 11));

  // Enable bit in CR1 for write-protection for kernel (prevent writes to
  // read-only pages)
  int64_t tmp;
  asm volatile("mov %%cr0, %0\n"
               "or %0, %c1\n"
               "mov %0, %%cr0"
               : "=&r"(tmp)
               : "i"(1 << 16));

  // Set up appropriate page permissions for the various segments linked into
  // the kernel
  for (char *c = &__text_start__; c != &__text_end__; c += PAGE_SIZE) {
    auto entry = kernel_page_tables.find(c);
    assert(entry != page_tables::iterator::end() && entry.exists() &&
           "no text page mapped?");
    assert(entry.present() && "text page not present?");
    *entry &= ~(attributes::XD | attributes::RW);
  }

  for (char *c = &__rodata_start__; c != &__rodata_end__; c += PAGE_SIZE) {
    auto entry = kernel_page_tables.find(c);
    assert(entry != page_tables::iterator::end() && entry.exists() &&
           "no rodata page mapped?");
    assert(entry.present() && "rodata page not present?");
    *entry = (*entry | attributes::XD) & ~attributes::RW;
  }

  for (char *c = &__data_start__; c != &__data_end__; c += PAGE_SIZE) {
    auto entry = kernel_page_tables.find(c);
    assert(entry != page_tables::iterator::end() && entry.exists() &&
           "no data page mapped?");
    assert(entry.present() && "data page not present?");
    *entry |= (uintptr_t)(attributes::XD | attributes::RW);
  }

  for (char *c = &__bss_start__; c != &__bss_end__; c += PAGE_SIZE) {
    auto entry = kernel_page_tables.find(c);
    assert(entry != page_tables::iterator::end() && "no bss page mapped?");
    assert(entry.present() && "bss page not present?");
    *entry |= (uintptr_t)(attributes::XD | attributes::RW);
  }

  // Give ourselves 10 pages (40KiB) worth of stack.
  constexpr static auto KERNEL_STACK_SIZE = 0x10 * PAGE_SIZE;
  const auto kernel_stack_top = (char *)kernel_stack_base - KERNEL_STACK_SIZE;
  assert(kernel_stack_top > &__bss_end__ && "bss and stack overlap!");
  for (char *c = kernel_stack_top; c != (char *)kernel_stack_base;
       c += PAGE_SIZE) {
    auto entry = kernel_page_tables.find(c);
    assert(entry != page_tables::iterator::end() && entry.exists() &&
           "no stack page mapped?");
    *entry |=
        (uintptr_t)(attributes::XD | attributes::RW | attributes::PRESENT);
  }
  // Map a guard page below the kernel stack
  auto entry = kernel_page_tables.find((char *)kernel_stack_base -
                                       KERNEL_STACK_SIZE - PAGE_SIZE);
  assert(entry != page_tables::iterator::end() && entry.exists() &&
         "no stack page mapped?");
  assert(entry.present() && "stack page not present?");
  *entry &= ~(uintptr_t)attributes::PRESENT;

  printf("Paging init: .text : 0x%p - 0x%p (0x%lx - 0x%lx) = EXEC | READ\n",
         &__text_start__, &__text_end__,
         kernel_page_tables.get_physical_address(&__text_start__),
         kernel_page_tables.get_physical_address(&__text_end__));
  printf("Paging init: .rodata : 0x%p - 0x%p (0x%lx - 0x%lx) = READ\n",
         &__rodata_start__, &__rodata_end__,
         kernel_page_tables.get_physical_address(&__rodata_start__),
         kernel_page_tables.get_physical_address(&__rodata_end__));
  printf("Paging init: .data : 0x%p - 0x%p (0x%lx - 0x%lx) = READ | WRITE\n",
         &__data_start__, &__data_end__,
         kernel_page_tables.get_physical_address(&__data_start__),
         kernel_page_tables.get_physical_address(&__data_end__));
  printf("Paging init: .bss : 0x%p - 0x%p (0x%lx - 0x%lx) = READ | WRITE\n",
         &__bss_start__, &__bss_end__,
         kernel_page_tables.get_physical_address(&__bss_start__),
         kernel_page_tables.get_physical_address(&__bss_end__));
  printf("Paging init: stack: 0x%p - 0x%p (0x%lx - 0x%lx) = READ | WRITE\n",
         (char *)kernel_stack_base - KERNEL_STACK_SIZE,
         (char *)kernel_stack_base,
         kernel_stack_base - KERNEL_STACK_SIZE - KERNEL_VMA_OFFSET,
         kernel_stack_base - KERNEL_VMA_OFFSET);

  asm volatile("mov %0, %%cr3\n\t"
               :
               : "r"((uintptr_t)&kernel_pml4 - KERNEL_VMA_OFFSET)
               : "memory");

  puts("Paging init: all kernel pages now protected");
}

void finish_init() {
  const auto physical_pages =
      pma::get_contiguous_physical_pages(MAX_DYNAMIC_TABLES);
  void *virtual_addr = kernel_page_tables.identity_map_pages_into_kernel_space(
      physical_pages, MAX_DYNAMIC_TABLES);
  dynamic_tables = (page_table *)virtual_addr;
  memset((void *)dynamic_tables, 0, sizeof(page_table) * MAX_DYNAMIC_TABLES);
  num_dynamic_tables_left = MAX_DYNAMIC_TABLES;
  vma::remove_from_free_list(virtual_addr,
                             sizeof(page_table) * MAX_DYNAMIC_TABLES);
  uint64_t dummy;
  asm volatile("mov %%cr3, %0\n"
               "mov %0, %%cr3"
               : "=r"(dummy)
               :
               : "memory");
}

#if 0
static page_entry_lookup_info
get_page_entry_info(page_tables &tables, uintptr_t virtual_page_address,
                    bool allocate_levels_if_null) {
  // We need to hit the page tables in the order
  // pml4 -> pml3 -> pml2 -> pml1 -> entry (ptr to physical page addr)
  //
  // For each table, we need 4KiB (0x1000 bytes = 8 bytes per entry * 512
  // entries) of memory.
  //
  // A page itself is 4KiB, so for each table that is null (not yet allocated),
  // we allocate a (pre-mapped) page and use that.

  const auto allocate_table_if_null = [&tables](volatile uintptr_t *
                                                entry) -> volatile uintptr_t * {
    if (reinterpret_cast<void *>(*entry) == nullptr) {
      assert(tables.allocate &&
             "tried to allocate before table allocator was initialized!");
      *entry = tables.allocate() | attributes::RW | attributes::PRESENT;
    }
    return entry;
  };

  page_entry_lookup_info handle{virtual_page_address};
  handle.tables[0] = tables.base; // PML4

  if (allocate_levels_if_null) {
    /* PML3 */ const auto *entry_in_pml4 =
        allocate_table_if_null(&handle.entry_at_level(4));
    handle.tables[1] = reinterpret_cast<page_table *>(
        (*entry_in_pml4 & ~PREFIX_MASK) + KERNEL_VMA_OFFSET);

    /* PML2 */ const auto *entry_in_pml3 =
        allocate_table_if_null(&handle.entry_at_level(3));
    handle.tables[2] = reinterpret_cast<page_table *>(
        (*entry_in_pml3 & ~PREFIX_MASK) + KERNEL_VMA_OFFSET);

    /* PML1 */ const auto *entry_in_pml2 =
        allocate_table_if_null(&handle.entry_at_level(2));
    handle.tables[3] = reinterpret_cast<page_table *>(
        (*entry_in_pml2 & ~PREFIX_MASK) + KERNEL_VMA_OFFSET);
  } else {
    if ((handle.tables[1] = reinterpret_cast<page_table *>(
             (handle.entry_at_level(4) & ~PREFIX_MASK) + KERNEL_VMA_OFFSET))) {
      if (((handle.tables[2] = reinterpret_cast<page_table *>(
                (handle.entry_at_level(3) & ~PREFIX_MASK) +
                KERNEL_VMA_OFFSET)))) {
        handle.tables[3] = reinterpret_cast<page_table *>(
            (handle.entry_at_level(2) & ~PREFIX_MASK) + KERNEL_VMA_OFFSET);
      }
    }
  }

  if (handle.tables[3])
    handle.entry = &handle.entry_at_level(1);

  return handle;
}
#endif

uintptr_t page_tables::get_physical_address(void *virtual_addr) const {
  const auto page_addr = (uintptr_t)virtual_addr & -PAGE_SIZE;
  const auto offset_in_page = (uintptr_t)virtual_addr - page_addr;
  const auto entry = find((void*)page_addr);
  assert(entry != page_tables::iterator::end() &&
         "couldn't find an entry for address!");
  if (!entry.present())
    fprintf(stderr, "entry not present for addr %p\n", virtual_addr);
  assert(entry.present() && "entry not found!");
  return entry.physical_page_address() + offset_in_page;
}

void *page_tables::map_range(uintptr_t physical_start, uintptr_t physical_end,
                             void *virtual_start, void *virtual_end,
                             attributes attrs) {
  assert((uintptr_t)virtual_start % memory::PAGE_SIZE == 0 &&
         (uintptr_t)virtual_end % memory::PAGE_SIZE == 0 &&
         "virtual addresses must be page-aligned!");
  assert(physical_start % memory::PAGE_SIZE == 0 &&
         physical_end % memory::PAGE_SIZE == 0 &&
         "physical addresses must be page-aligned!");
  assert((physical_end - physical_start) ==
             ((uintptr_t)virtual_end - (uintptr_t)virtual_start) &&
         "physical and virtual address range sizes must match!");
  auto it = get(virtual_start);
  auto v = reinterpret_cast<uintptr_t>(virtual_start);
  for (auto p = physical_start; p < physical_end;
       p += PAGE_SIZE, v += PAGE_SIZE) {
    assert(it.virtual_page_address() == v &&
           "iterator and virtual address out of sync!");
    map_page(p, it, attrs);
    while (++it == page_tables::iterator::end(it.level()))
      it.ascend();
    while (it.level() != 1)
      it.allocate_and_descend(allocate);
  }
  return virtual_start;
}

void *page_tables::map_range_size(uintptr_t physical_start, void *virtual_start,
                                  size_t size, attributes attrs) {
  // Align size to a multiple of a page
  size = kstd::align_to(size, PAGE_ALIGN);
  return map_range(physical_start, physical_start + size, virtual_start,
                   (void *)((uintptr_t)virtual_start + size), attrs);
}

void *page_tables::map_page(uintptr_t physical_page, void *virtual_page,
                            attributes attrs) {
  assert((uintptr_t)virtual_page % memory::PAGE_SIZE == 0 &&
         "virtual address must be page-aligned!");
  return map_page(physical_page, get(virtual_page), attrs);
}

void *page_tables::map_page(uintptr_t physical_page, page_tables::iterator it,
                            attributes attrs) {
  void *virtual_page = reinterpret_cast<void *>(it.virtual_page_address());
  assert((uintptr_t)virtual_page % memory::PAGE_SIZE == 0 &&
         "virtual address must be page-aligned!");
  assert(physical_page % memory::PAGE_SIZE == 0 &&
         "physical address must be page-aligned!");
  assert(it.level() == 1 && "iterator must point to a page table entry!");
  assert(it != page_tables::iterator::end() && it.exists() &&
         "lookup/allocation failed?");

  if (it.present())
    kstd::panic("Tried to map already-present page!\n"
                "Virtual :%p\n"
                "Physical:%p\n"
                "Entry   :%lx\n",
                (void *)virtual_page, (void *)physical_page, *it);

  *it =
      physical_page | (uintptr_t)attrs | (uintptr_t)attributes::PRESENT;

  invlpg((uintptr_t)virtual_page);
  return virtual_page;
}

void page_tables::unmap_page(void *virtual_page) {
  assert((uintptr_t)virtual_page % memory::PAGE_SIZE == 0 &&
         "virtual address should be page-aligned!");
  auto entry = find(virtual_page);

  if (entry == page_tables::iterator::end() || !entry.exists() ||
      !entry.present())
    kstd::panic("Tried to unmap non-present page\n"
                "Virtual:%p",
                (void *)virtual_page);

  *entry &= ~attributes::PRESENT;
  invlpg((uintptr_t)virtual_page);
}

void page_tables::unmap_range(void *virtual_start, void *virtual_end) {
  assert((uintptr_t)virtual_start % memory::PAGE_SIZE == 0 &&
         "virtual start address should be page-aligned!");
  assert((uintptr_t)virtual_end % memory::PAGE_SIZE == 0 &&
         "virtual end address should be page-aligned!");
  for (char *p = (char *)virtual_start; p != virtual_end;
       p += memory::PAGE_SIZE)
    unmap_page(p);
}

void *
page_tables::identity_map_pages_into_kernel_space(uintptr_t starting_address,
                                                  size_t n, attributes attrs) {
  assert(n > 0 && "trying to map 0 pages?");
  return identity_map_page_range_into_kernel_space(starting_address,
                                                   n * PAGE_SIZE, attrs);
}

void *page_tables::identity_map_page_range_into_kernel_space(uintptr_t address,
                                                             size_t size,
                                                             attributes attrs) {
  return map_range_size(address, (void *)(address + KERNEL_VMA_OFFSET), size,
                        attrs);
}

page_tables *allocate_user_space_page_tables() {
 return nullptr;
}

void page_tables::dump_to_file(FILE *out) const {
  for (uint64_t i = 0; i < NUM_PAGE_TABLE_ENTRIES; ++i) {
    const uintptr_t pml4_entry = (*base)[i];
    if ((pml4_entry & attributes::PRESENT) == 0)
      continue;

    const page_table *pml3 = (page_table *)(pml4_entry & ~PREFIX_MASK);
    for (uint64_t j = 0; j < NUM_PAGE_TABLE_ENTRIES; ++j) {
      const uintptr_t pml3_entry = (*pml3)[j];
      if ((pml3_entry & attributes::PRESENT) == 0)
        continue;

      const page_table *pml2 = (page_table *)(pml3_entry & ~PREFIX_MASK);
      for (uint64_t k = 0; k < NUM_PAGE_TABLE_ENTRIES; ++k) {
        const uintptr_t pml2_entry = (*pml2)[k];
        if ((pml2_entry & attributes::PRESENT) == 0)
          continue;

        const page_table *pml1 = (page_table *)(pml2_entry & ~PREFIX_MASK);
        for (uint64_t l = 0; l < NUM_PAGE_TABLE_ENTRIES; ++l) {
          const uintptr_t pml1_entry = (*pml1)[l];
          if ((pml1_entry & attributes::PRESENT) == 0)
            continue;
          const uintptr_t virtual_addr =
              kstd::sign_extend((l << 12) | (k << 21) | (j << 30) | (i << 39),
                                /*num_source_bits=*/42);
          const uintptr_t physical_addr = pml1_entry & ADDRESS_MASK;
          const char *rw_flags = (pml1_entry & attributes::RW) ? "rw" : "r";
          const char *x_flags = (pml1_entry & attributes::XD) ? "" : "x";
          fprintf(out, "Virtual 0x%p -> Physical 0x%p (%s%s)\n",
                  (void *)virtual_addr, (void *)physical_addr, rw_flags,
                  x_flags);
        }
      }
    }
  }
}
} // namespace paging
