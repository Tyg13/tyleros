#include "memory.h"

#include "alloc.h"
#include "pma.h"
#include "util.h"
#include "vma.h"

#include <assert.h>
#include <stddef.h>

namespace memory {

static void sort_memory_map();
static void dump_memory_map();

uint32_t g_num_of_memory_map_entries = 0;
memory_map g_memory_map;

void early_init(uint32_t memory_map_base, uint32_t num_memory_map_entries) {
  assert(num_memory_map_entries < MAX_MEMORY_MAP_ENTRIES &&
         "too many memory map entries!");
  memcpy(&g_memory_map, reinterpret_cast<memory_map *>(memory_map_base),
         sizeof(memory_map_entry) * num_memory_map_entries);
  g_num_of_memory_map_entries = num_memory_map_entries;

  sort_memory_map();
}

void finish_init() {
  pma::early_init();
  vma::init();
  vma::dump_free_list();
  alloc::init();
}

memory_map &get_memory_map() {
  assert(g_num_of_memory_map_entries != 0 && "memory map not initialized!");
  return g_memory_map;
}

void sort_memory_map() {
  // Sort the physical memory map returned by the bootloader, putting the
  // largest usable regions first in our list
  kstd::insertion_sort(g_memory_map, g_num_of_memory_map_entries,
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
        g_memory_map, g_num_of_memory_map_entries, [=](auto &entry) {
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
    const memory_map_entry &entry = g_memory_map[i];
    printf("memory_map[%d]:\n"
           "\ttype: %s\n"
           "\tbase: %lx\n"
           "\tsize: %lx\n",
           i, entry.type_str(), entry.base, entry.length);
  }
};
} // namespace memory
