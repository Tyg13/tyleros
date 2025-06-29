#ifndef MEMORY_H
#define MEMORY_H

#include "util.h"
#include <stddef.h>
#include <stdint.h>

extern char __text_start__, __text_end__;
extern char __rodata_start__, __rodata_end__;
extern char __data_start__, __data_end__;
extern char __bss_start__, __bss_end__;

namespace memory {
// 4KiB pages
constexpr static size_t PAGE_SIZE = 0x1000;
constexpr static auto PAGE_ALIGN = kstd::Align{PAGE_SIZE};

constexpr static ptrdiff_t KERNEL_SIZE = 0x100 * PAGE_SIZE;
constexpr static ptrdiff_t KERNEL_VMA_OFFSET = 0xFFFFFFFF80000000;
constexpr static uintptr_t KERNEL_LMA_START = 0x100000;
constexpr static uintptr_t KERNEL_LMA_END = KERNEL_LMA_START + KERNEL_SIZE;
constexpr static uintptr_t KERNEL_VMA_START = KERNEL_LMA_START + KERNEL_VMA_OFFSET;
constexpr static uintptr_t KERNEL_VMA_END = KERNEL_VMA_START + KERNEL_SIZE;

struct memory_map_entry {
  enum type : uint32_t {
    usable = 1,
    reserved = 2,
    reclaimable = 3,
    nvs = 4,
    badmemory = 5,
  };
  uintptr_t base;
  uint64_t length;
  type type;
  uint32_t extended_attributes = 1;

  const char *type_str() const {
    switch (type) {
    case memory_map_entry::usable:
      return "usable";
    case memory_map_entry::reserved:
      return "reserved";
    case memory_map_entry::reclaimable:
      return "reclaimable";
    case memory_map_entry::nvs:
      return "nvs";
    case memory_map_entry::badmemory:
      return "badmemory";
    default:
      return "<error>";
    }
  }
} __attribute((packed));

static constexpr auto MAX_MEMORY_MAP_ENTRIES = 16;
using memory_map = memory_map_entry[MAX_MEMORY_MAP_ENTRIES];

void early_init(uint32_t memory_map_base, uint32_t num_memory_map_entries);
void finish_init();
memory_map &get_memory_map();
} // namespace memory

#endif
