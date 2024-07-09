#ifndef MEMORY_H
#define MEMORY_H

#include "util.h"
#include <stddef.h>
#include <stdint.h>

extern char __KERNEL_VMA_START__;
extern char __KERNEL_VMA_END__;
extern char __KERNEL_LMA_START__;
extern char __KERNEL_LMA_END__;
extern char __text_start__, __text_end__;
extern char __rodata_start__, __rodata_end__;
extern char __data_start__, __data_end__;
extern char __bss_start__, __bss_end__;

namespace memory {
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

using memory_map = memory_map_entry[];

// 4KiB pages
constexpr static size_t PAGE_SIZE = 0x1000;
constexpr static auto PAGE_ALIGN = kstd::Align{PAGE_SIZE};

void init(uint32_t memory_map_base, uint32_t num_memory_map_entries);
memory_map &get_memory_map();
} // namespace memory

#endif
