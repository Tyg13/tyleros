#include "filesystem.h"

#include "alloc.h"
#include "debug.h"
#include "floppy.h"
#include "util.h"

#include <algorithm>
#include <assert.h>
#include <string.h>

namespace fs {

bios_parameter_block *bpb = nullptr;
directory_entry *root_dir = nullptr;
uint8_t *fat = nullptr;

static void read_boot_param_block();
static void read_root_directory();
static const directory_entry *resolve_path(const char *path);

void init() {
  read_boot_param_block();
  read_root_directory();
  fat = alloc::array_of<uint8_t>(bpb->sectors_per_fat * bpb->bytes_per_sector);
  debug::puts("fs: initialized");
  const auto *kernel = resolve_path("/KERNEL");
  assert(kernel && "couldn't get kernel file!");
  debug::printf("kernel size: %uB\n", kernel->size);
}

void read_boot_param_block() {
  bpb = alloc::one<bios_parameter_block>();
  read_floppy_or_fail(bpb, 0, 1);
}

void read_root_directory() {
  assert(bpb && "bios param block must be populated before root directory!");
  const auto fat_sectors = bpb->sectors_per_fat * bpb->num_of_fats;
  const auto root_directory_sector = bpb->num_of_reserved_sectors + fat_sectors;
  const auto root_directory_size =
      sizeof(directory_entry) * bpb->num_of_directory_entries;
  const auto root_directory_sector_count =
      root_directory_size / bpb->bytes_per_sector;

  root_dir = alloc::array_of<directory_entry>(bpb->num_of_directory_entries);
  read_floppy_or_fail(root_dir, root_directory_sector,
                      root_directory_sector_count);
}

static const directory_entry *resolve_path(const char *path) {
  assert(root_dir && "can't resolve path before root dir is populated!");
  assert(bpb && "can't resolve path before bios param block is populated!");
  assert(path && path[0] == '/' && "path null or not rooted!");
  const char *rest = &path[1];

  char fragment[directory_entry::file_name_size + 2] = {0};
  auto read_next_fragment = [&](auto &rest) {
    const char *dir_sep = strchr(rest, '/');
    const auto rest_size = dir_sep ? dir_sep - rest : strlen(rest);
    size_t fragment_size = std::min(sizeof(fragment) - 1, rest_size);

    strncpy(fragment, rest, fragment_size);
    for (size_t i = fragment_size; i < directory_entry::file_name_size + 1;
         ++i) {
      fragment[i] = ' ';
    }
    fragment[directory_entry::file_name_size + 1] = '\0';

    rest = rest + fragment_size;
    return fragment;
  };

  read_next_fragment(rest);

  const directory_entry *file_entry = nullptr;
  const directory_entry *directory = root_dir;
  const auto directory_size = bpb->num_of_directory_entries;
  for (auto i = 0; i < directory_size; ++i) {
    const auto &entry = directory[i];
    if (strcmp(entry.short_file_full_name, fragment) == 0) {
      // Path fully resolved
      if (rest[0] == '\0') {
        assert((entry.attributes & directory_entry::DIRECTORY) == 0 &&
               "file should not be a directory!");
        file_entry = &entry;
        break;
      }
      assert(entry.attributes & directory_entry::DIRECTORY &&
             "path fragment not directory!");

      read_next_fragment(rest);
      i = 0;
      directory = &entry;
    } else {
      debug::printf("searching for '%s'\n"
                    "not found:    '%s'\n",
                    fragment, entry.short_file_full_name);
    }
  }
  assert(file_entry && "couldn't resolve path");
  return file_entry;
}

void dump_dir(const char *path) {
  assert(path && "can't dump null path!");
  if (strcmp(path, "/") != 0)
    return;

  char file_name[directory_entry::file_name_size + 1] = {'\0'};
  const directory_entry *directory = root_dir;
  const auto directory_size = bpb->num_of_directory_entries;
  for (auto i = 0; i < directory_size; ++i) {
    const auto &entry = directory[i];
    if (strcmp(entry.short_file_full_name, "") != 0) {
      strncpy(file_name, entry.short_file_full_name, directory_entry::file_name_size);
      debug::printf("/%s: size=%uB\n", file_name, entry.size);
      memset(file_name, 0, directory_entry::file_name_size);
    }
  }
}

void *read_file(const char *path) {
  assert(bpb && "must have bios param block to read files!");
  assert(fat && "must have fat storage to read files!");
  const directory_entry *file_entry = resolve_path(path);
  auto buffer = alloc::array_of<char>(file_entry->size);
  const uint16_t first_fat_sector = bpb->num_of_reserved_sectors;
  const uint16_t num_of_fat_sectors = bpb->sectors_per_fat * bpb->num_of_fats;
  const uint16_t size_of_root_dir =
      bpb->num_of_directory_entries * sizeof(directory_entry);
  const auto num_of_root_dir_sectors =
      kstd::div_ceil(size_of_root_dir, bpb->bytes_per_sector);
  const auto first_data_sector =
      first_fat_sector + num_of_fat_sectors + num_of_root_dir_sectors;

  auto offset = 0;
  auto cluster = file_entry->cluster_num_lo;
  uint16_t fat_value = 0;
  while (cluster < 0xFF8) {
    const auto fat_offset = cluster + (cluster / 2);
    const auto fat_sector =
        first_fat_sector + (fat_offset / bpb->bytes_per_sector);
    const auto entry_offset = fat_offset % bpb->bytes_per_sector;
    read_floppy_or_fail(fat, fat_sector, 1);

    const auto data_sector =
        ((cluster - 2) * bpb->sectors_per_cluster) + first_data_sector;
    read_floppy_or_fail(buffer + offset, data_sector, 1);

    offset += bpb->bytes_per_sector;

    const auto *fat_entry = reinterpret_cast<uint16_t *>(&fat[entry_offset]);
    fat_value = *fat_entry;
    cluster = (cluster & 1) ? fat_value >> 4 : fat_value & 0x0FFF;
  }

  return buffer;
}
} // namespace fs
