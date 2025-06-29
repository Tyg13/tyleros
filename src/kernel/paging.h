#ifndef PAGING_H
#define PAGING_H

#include "memory.h"
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <type_traits>

namespace paging {

void early_init();
void enable_kernel_page_protection(uintptr_t kernel_stack_base);
void finish_init();

// Each prefix is 9 bits wide.
// Each page (and consequently, each page table) must be aligned to 4KiB (1 <<
// 12);
constexpr unsigned int LOG2_PAGE_TABLE_SIZE = 9;
constexpr unsigned int LOG2_PAGE_ALIGN = 12;
constexpr uintptr_t PREFIX_MASK = (1 << LOG2_PAGE_TABLE_SIZE) - 1;
constexpr uintptr_t ADDRESS_MASK = 0x000FFFFFFFFFF000;

struct attributes {
  enum vals : uint64_t {
    NONE = 0ULL,
    // If set, the page is actually in physical memory. If unset, it may not be
    // (e.g. swapped out)
    PRESENT = 1ULL << 0,
    // If set, the page is read/write; otherwise, it is read-only.
    RW = 1ULL << 1,
    // If set, the page is accessible from user-code.
    USER = 1ULL << 2,
    // If set, the page is not executable.
    XD = 1ULL << 63,
  } v;
  static auto constexpr ALL_MASK = PRESENT | RW | USER | XD;
  explicit attributes(vals v) : v{v} {}
  explicit operator vals() const { return v; }
  attributes(uint64_t v) : v{v} {}
  operator uint64_t() const { return v; }
};
inline attributes::vals operator&(attributes::vals a, attributes::vals b) {
  return attributes::vals{(uint64_t)a & (uint64_t)b};
}
inline attributes::vals operator|(attributes::vals a, attributes::vals b) {
  return attributes::vals{(uint64_t)a | (uint64_t)b};
}
inline uint64_t operator~(attributes a) { return ~(uint64_t)a; }

static constexpr auto NUM_PAGE_TABLE_ENTRIES = 512;
using page_table = volatile uintptr_t[NUM_PAGE_TABLE_ENTRIES];
using page_table_allocator = uintptr_t();

class page_entry_lookup_info {
  size_t indices[4] = {0};

public:
  // Address of the PML1 entry that was addressed by `address`
  volatile uintptr_t *entry = nullptr;
  // Addresses of the PML4,  PML3,  PML2, PML1 tables
  //                  (PML4) (PDPT) (PDT) (PT)
  //                  [0]    [1]    [2]   [3]
  volatile page_table *tables[4] = {nullptr};

  page_entry_lookup_info(uintptr_t address) {
    address >>= LOG2_PAGE_ALIGN;
    for (int i = 0; i < 4; ++i) {
      indices[3 - i] = address & PREFIX_MASK;
      address >>= LOG2_PAGE_TABLE_SIZE;
    }
  }

  bool exists() const {
    assert((!entry ||
            (entry && tables[0] && tables[1] && tables[2] && tables[3])) &&
           "inconsistent lookup info!");
    return entry;
  }

  bool present() const {
    assert(entry && "entry doesn't exist!");
    return (uintptr_t)*entry & paging::attributes::PRESENT;
  }

  volatile uintptr_t &operator*() { return *entry; }
  uintptr_t operator*() const { return *entry; }

  volatile uintptr_t &entry_at_level(unsigned n) {
    assert(1 <= n && n <= 4 && "valid levels are 1 - 4");
    return (*tables[4 - n])[indices[4 - n]];
  }
  uintptr_t entry_at_level(unsigned n) const {
    return const_cast<page_entry_lookup_info *>(this)->entry_at_level(n);
  }
  uintptr_t physical_page_address() const { return *entry & ADDRESS_MASK; }
  void set_attributes(attributes attrs) {
    *entry = (*entry & ~attributes::ALL_MASK) | (uintptr_t)attrs;
  }
};

class page_tables {
  template <typename table_ty>
  class _iterator {
    using non_const_table_ty = std::remove_const_t<table_ty>;
    using const_table_ty = std::add_const_t<table_ty>;

    table_ty *tables[4] = {nullptr, nullptr, nullptr, nullptr};
    unsigned indices[4] = {0, 0, 0, 0};
    unsigned _level = 3;

    explicit _iterator() = default;
    friend class _iterator<non_const_table_ty>;
  public:
    explicit _iterator(table_ty *base) {
      tables[3] =base;
    }
    explicit _iterator(table_ty *base, const void *addr) : _iterator(base) {
      auto address = reinterpret_cast<uintptr_t>(addr);
      address >>= LOG2_PAGE_ALIGN;
      for (int level = 1; level <= 4; ++level) {
        indices[level - 1] = address & PREFIX_MASK;
        address >>= LOG2_PAGE_TABLE_SIZE;
      }
    }
    static _iterator end(unsigned level = 4) {
      _iterator ret;
      ret._level = level - 1;
      ret.indices[ret._level] = NUM_PAGE_TABLE_ENTRIES;
      return ret;
    }
    _iterator(const _iterator &) = default;
    _iterator& operator=(const _iterator&) = default;

    operator _iterator<const_table_ty>() const {
      _iterator<const_table_ty> ret;
      memcpy(&ret, this, sizeof(_iterator));
      return ret;
    }

    unsigned level() const { return _level + 1; }
    volatile uintptr_t &operator*() {
      auto *table = reinterpret_cast<non_const_table_ty *>(tables[_level]);
      unsigned idx = indices[_level];
      assert(table && "empty page table!");
      assert(idx < NUM_PAGE_TABLE_ENTRIES &&
             "index into page tables out of bounds!");
      return (*table)[idx];
    }
    const volatile uintptr_t &operator*() const {
      const auto *table = reinterpret_cast<const_table_ty *>(tables[_level]);
      unsigned idx = indices[_level];
      assert(table && "empty page table!");
      assert(idx < NUM_PAGE_TABLE_ENTRIES &&
             "index into page tables out of bounds!");
      return (*table)[idx];
    }

    bool exists() const { return tables[_level] != nullptr; }
    bool present() const { return **this & attributes::PRESENT; }

    uintptr_t physical_page_address() const {
      assert(level() == 1 && "can't get physical addr of non level 1 page!");
      return **this & ADDRESS_MASK;
    }

    uintptr_t virtual_page_address() const {
      return kstd::sign_extend(
          ((uintptr_t)indices[3] << 39) | ((uintptr_t)indices[2] << 30) |
              ((uintptr_t)indices[1] << 21) | ((uintptr_t)indices[0] << 12),
          /*num_source_bits=*/42);
    }

    bool descend() {
      if (level() == 1 || indices[_level] >= NUM_PAGE_TABLE_ENTRIES ||
          !exists() || !present())
        return false;

      tables[--_level] = reinterpret_cast<table_ty *>(
          (**this & ~PREFIX_MASK) + memory::KERNEL_VMA_OFFSET);
      return true;
    }

    void allocate_and_descend(page_table_allocator allocate) {
      assert(level() > 1 &&
             "can't allocate and descend on level 1 page iterator!");
      assert(indices[_level] < NUM_PAGE_TABLE_ENTRIES &&
             "can't allocate on end iterator!");

      auto addr = (uintptr_t)nullptr;

      if (tables[_level] != nullptr)
        addr = **this & ~PREFIX_MASK;

      if (addr == (uintptr_t) nullptr) {
        addr = allocate();
        **this = addr | attributes::RW | attributes::PRESENT;
      }

      assert(addr % memory::PAGE_ALIGN == 0 && "address isn't page-aligned?");
      tables[--_level] =
          reinterpret_cast<table_ty *>(addr + memory::KERNEL_VMA_OFFSET);
    }

    void ascend() {
      assert(level() < 4 && "can't ascend from level 4 page iterator");
      indices[_level++] = 0;
    }

    _iterator &operator++() {
      advance(1);
      return *this;
    }

    void advance(unsigned n = 1) {
      assert(indices[_level] + n <= NUM_PAGE_TABLE_ENTRIES &&
             "advanced out of bounds!");
      indices[_level] += n;
    }

    friend bool operator==(const _iterator &lhs, const _iterator &rhs) {
      return lhs._level == rhs._level &&
             lhs.indices[lhs._level] == rhs.indices[lhs._level];
    }

    friend bool operator!=(const _iterator &lhs, const _iterator &rhs) {
      return !(lhs == rhs);
    }
  };

public:
  page_table *base = nullptr;
  page_table_allocator *allocate = nullptr;

  using iterator = _iterator<page_table>;
  using const_iterator = _iterator<const page_table>;

  iterator begin() { return iterator{base}; }
  const_iterator begin() const { return const_iterator{base}; }

  iterator find(const void *page) {
    auto cursor = iterator(base, page);
    while (cursor.descend())
      ;
    if (cursor.level() != 1)
      return page_tables::iterator::end();
    return cursor;
  }
  const_iterator find(const void *page) const {
    return const_cast<page_tables *>(this)->find(page);
  }

  iterator get(const void *page) {
    assert(allocate &&
           "tried to allocate before table allocator was initialized!");
    auto cursor = iterator(base, page);
    for (unsigned level = 4; level > 1; --level)
      cursor.allocate_and_descend(allocate);

    assert(cursor.level() == 1 && "didn't find a level 1 page?");
    return cursor;
  }
  const_iterator get(const void *page) const {
    return const_cast<page_tables *>(this)->get(page);
  }

  void *map_range(uintptr_t physical_start, uintptr_t physical_end,
                  void *virtual_start, void *virtual_end,
                  attributes attrs = attributes::RW | attributes::XD);
  void *map_range_size(uintptr_t physical_start, void *virtual_start,
                       size_t size, attributes attrs = attributes::RW | attributes::XD);
  void *map_page(uintptr_t physical_page, iterator it,
                 attributes attrs = attributes::RW | attributes::XD);
  void *map_page(uintptr_t physical_page, void *virtual_page,
                 attributes attrs = attributes::RW | attributes::XD);
  void unmap_page(void *virtual_page);
  void unmap_range(void *virtual_start, void *virtual_end);
  void *identity_map_pages_into_kernel_space(uintptr_t address, size_t n,
                                             attributes attrs = attributes::RW |
                                                                attributes::XD);
  void *identity_map_page_into_kernel_space(uintptr_t address,
                                            attributes attrs = attributes::RW |
                                                               attributes::XD) {
    return identity_map_pages_into_kernel_space(address, 1, attrs);
  }
  static void *identity_map_address_to_kernel_space(uintptr_t address) {
    return (void *)(address + memory::KERNEL_VMA_OFFSET);
  }
  template <typename T>
  T *identity_map_pages_into_kernel_space(T *address, size_t n,
                                         attributes attrs = attributes::RW |
                                                            attributes::XD) {
    return (T *)identity_map_pages_into_kernel_space((uintptr_t)address, n,
                                                     attrs);
  }
  template <typename T>
  T *identity_map_page_into_kernel_space(T *address,
                                         attributes attrs = attributes::RW |
                                                            attributes::XD) {
    return (T *)identity_map_page_into_kernel_space((uintptr_t)address, attrs);
  }
  template <typename T>
  static T *identity_map_address_to_kernel_space(T *address) {
    return (T *)((uintptr_t)address + memory::KERNEL_VMA_OFFSET);
  }
  void *identity_map_containing_page_into_kernel_space(uintptr_t address) {
    return identity_map_page_into_kernel_space(address & -memory::PAGE_SIZE);
  }
  void *identity_map_page_range_into_kernel_space(uintptr_t address,
                                                  size_t size,
                                                  attributes attrs = attributes::RW |
                                                               attributes::XD);

  uintptr_t get_physical_address(void *virtual_addr) const;

  void dump_to_file(FILE *out) const;
  __attribute__((used)) void dump_to_stderr() const {
    return dump_to_file(stderr);
  }
};

extern page_tables kernel_page_tables;
page_tables *allocate_user_space_page_tables();

} // namespace paging

#endif
