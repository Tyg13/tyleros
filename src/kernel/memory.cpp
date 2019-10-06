#include "memory.h"
#include "util.h"

const auto & g_num_of_memory_map_entries = * reinterpret_cast<uint32_t *>            (0x1000);
      auto & g_memory_map                = * reinterpret_cast<memory_map_entry (*)[]>(0x1008);

void sort_memory_map() {
   const auto better = [](const auto & lhs, const auto & rhs) {
      const auto entry_is_usable =
         [](const auto & entry) { return entry.type == memory_map_entry::type::usable; };
      if (entry_is_usable(lhs)) {
         if (entry_is_usable(rhs)) {
            // If both are usable regions, the larger one is better.
            return lhs.length > rhs.length;
         } else {
            // If the rhs is unusable, the lhs is better.
            return true;
         }
      } else {
         // If the lhs is unusable, the rhs is better.
         return false;
      }
   };
   for (int i = 0; i < (int) g_num_of_memory_map_entries; ++i) {
      for (int j = i; j < (int) g_num_of_memory_map_entries; ++j) {
         if (better(g_memory_map[j], g_memory_map[i])) {
            swap(g_memory_map[i], g_memory_map[j]);
         }
      }
   }
}
