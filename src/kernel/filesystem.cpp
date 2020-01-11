#include "filesystem.h"

#include "util.h"
#include "floppy.h"

#include <string.h>

bios_parameter_block * bpb      = nullptr;
directory_entry      * root_dir = nullptr;
char                 * fat      = nullptr;

static void read_boot_sector();
static void read_root_directory();

void init_filesystem() {
   read_boot_sector();
   read_root_directory();
}

void read_boot_sector() {
   bpb = new bios_parameter_block;
   read_floppy(bpb, 0, 1);
}

void read_root_directory() {
   const auto fat_sectors                 = bpb->sectors_per_fat * bpb->num_of_fats;
   const auto root_directory_sector       = bpb->num_of_reserved_sectors + fat_sectors;
   const auto root_directory_size         = sizeof(directory_entry) * bpb->num_of_directory_entries;
   const auto root_directory_sector_count = root_directory_size / bpb->bytes_per_sector;

   root_dir = new directory_entry[bpb->num_of_directory_entries];
   read_floppy(root_dir, root_directory_sector, root_directory_sector_count);
}

static const directory_entry * resolve_path(const char * path) {
   assert(path && path[0] == '/', "Tried to read file: %s", path ? path : "null");
   const char * rest = &path[1];

   char fragment[sizeof(directory_entry::short_file_full_name) + 1];
   auto read_next_fragment = [&](auto & rest) {
      const char * dir_sep = strchr(rest, '/');
      const auto rest_size = dir_sep ? dir_sep - rest : strlen(rest);
      size_t fragment_size = min(sizeof(fragment) - 1, rest_size);

      strncpy(fragment, rest, fragment_size);
      fragment[fragment_size + 1] = '\0';

      rest = rest + fragment_size;
      return fragment;
   };

   read_next_fragment(rest);

   const directory_entry * file_entry = nullptr;
   const directory_entry * directory = root_dir;
   const auto directory_size = bpb->num_of_directory_entries;
   for (auto i = 0; i < directory_size; ++i) {
      const auto & entry = directory[i];
      if (strcmp(entry.short_file_full_name, fragment) == 0) {
         // Path fully resolved
         if (rest[0] == '\0') {
            file_entry = &entry;
            break;
         }
         assert(entry.attributes & directory_entry::DIRECTORY,
                "Path fragment not directory: %s (%s)", fragment, rest);

         read_next_fragment(rest);
         i = 0;
         directory = &entry;
      }
   }
   assert(file_entry != nullptr, "Couldn't resolve path %s", path);
   return file_entry;
}

void * read_file(const char * path) {
   const directory_entry * file_entry = resolve_path(path);
   auto buffer = new char[file_entry->size];
   if (!fat) {
      fat = new char[bpb->sectors_per_fat * bpb->bytes_per_sector];
   }
   const auto first_fat_sector        = bpb->num_of_reserved_sectors;
   const auto num_of_fat_sectors      = bpb->sectors_per_fat * bpb->num_of_fats;
   const auto size_of_root_dir        = bpb->num_of_directory_entries * sizeof(directory_entry);
   const auto num_of_root_dir_sectors = div_round_up(size_of_root_dir, bpb->bytes_per_sector);
   const auto first_data_sector       = first_fat_sector + num_of_fat_sectors + num_of_root_dir_sectors;

   auto     offset    = 0;
   auto     cluster   = file_entry->cluster_num_lo;
   uint16_t fat_value = 0;
   while (cluster < 0xFF8) {
      const auto fat_offset   = cluster + (cluster / 2);
      const auto fat_sector   = first_fat_sector + (fat_offset / bpb->bytes_per_sector);
      const auto entry_offset = fat_offset % bpb->bytes_per_sector;
      read_floppy(fat, fat_sector, 1);

      const auto data_sector = ((cluster - 2) * bpb->sectors_per_cluster) + first_data_sector;
      read_floppy(buffer + offset, data_sector, 1);

      offset += bpb->bytes_per_sector;

      const auto * fat_entry = reinterpret_cast<uint16_t *>(&fat[entry_offset]);
      fat_value = *fat_entry;
      cluster = (cluster & 1) ? fat_value >> 4
                              : fat_value & 0x0FFF;
   }

   return buffer;
}
