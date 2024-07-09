#include "memory.h"

#include "alloc.h"
#include "debug.h"
#include "low_memory_allocator.h"
#include "pma.h"
#include "util.h"
#include "vma.h"

#include <assert.h>
#include <stddef.h>

namespace memory {

static void sort_memory_map();
static void dump_memory_map();

uint32_t g_num_of_memory_map_entries = 0;
memory_map *g_memory_map = nullptr;

void init(uint32_t memory_map_base, uint32_t num_memory_map_entries) {
  g_memory_map = reinterpret_cast<memory_map *>(memory_map_base);
  g_num_of_memory_map_entries = num_memory_map_entries;

  sort_memory_map();
  pma::init();
  vma::init();
  alloc::init();

  const uintptr_t low_mem_start = low_memory::get_avail_low_mem_start();
  const uintptr_t low_mem_end = low_memory::get_avail_low_mem_end();
  debug::puts("Memory initialized:");
  debug::printf("    Low memory:  0x%0lx - 0x%0lx (%u bytes)\n", low_mem_start,
                low_mem_end, (uint32_t)low_mem_end - (uint32_t)low_mem_start);
  for (unsigned i = 0; i < g_num_of_memory_map_entries; ++i) {
    debug::printf("    Mem%d region: 0x%0lx - 0x%0lx (%lu bytes): %s\n", i,
                  (*g_memory_map)[i].base,
                  (*g_memory_map)[i].base + (*g_memory_map)[i].length,
                  (*g_memory_map)[i].length, (*g_memory_map)[i].type_str());
  }
}

memory_map &get_memory_map() {
  assert(g_memory_map != nullptr && "memory map not initialized!");
  return *g_memory_map;
}

void sort_memory_map() {
  // Sort the physical memory map returned by the bootloader, putting the
  // largest usable regions first in our list
  kstd::insertion_sort(*g_memory_map, g_num_of_memory_map_entries,
                       [](auto &&lhs, auto &&rhs) {
                         constexpr auto entry_is_usable = [](auto &&entry) {
                           return entry.type == memory_map_entry::usable;
                         };
                         if (!entry_is_usable(lhs)) {
                           // If the lhs is unusable, the rhs is better.
                           return false;
                         }
                         if (!entry_is_usable(rhs)) {
                           // If the rhs is unusable, the lhs is better.
                           return true;
                         }
                         // If both are usable regions, the larger one is
                         // better.
                         return lhs.length > rhs.length;
                       });
  const auto mark_unusable = [](auto start, auto end) {
    kstd::transform(
        *g_memory_map, g_num_of_memory_map_entries, [=](auto &entry) {
          if ((uintptr_t)start <= entry.base && entry.base < (uintptr_t)end) {
            entry.length -= end - entry.base;
            entry.base = end;
            return true;
          }
          return false;
        });
  };
  // We can't actually use 0x0 - 0x500 because this memory is owned by the BIOS.
  mark_unusable(0x0, 0x500);

  // The kernel is located from 0x100000 - 0x300000
  mark_unusable(0x100000, 0x300000);

  dump_memory_map();
}

void dump_memory_map() {
  for (auto i = 0; i < (int)g_num_of_memory_map_entries; ++i) {
    const memory_map_entry &entry = (*g_memory_map)[i];
    debug::printf("memory_map[%d]:\n"
                  "\ttype: %s\n"
                  "\tbase: %lx\n"
                  "\tsize: %lx\n",
                  i, entry.type_str(), entry.base, entry.length);
  }
};
} // namespace memory
