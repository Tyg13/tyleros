#include "filesystem.h"

#include "alloc.h"
#include "floppy.h"
#include "util/unique_ptr.h"
#include "util.h"

#include <assert.h>
#include <ctype.h>
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
  fat = alloc::array_of<uint8_t>(2 * BYTES_PER_SECTOR);
  puts("fs: initialized");
  const auto *kernel = resolve_path("/KERNEL");
  assert(kernel && "couldn't get kernel file!");
  fprintf(stderr, "kernel size: %uB\n", kernel->size);
}

void read_boot_param_block() {
  bpb = alloc::one<bios_parameter_block>();
  read_floppy_or_fail(bpb, 0, 1);
  assert(bpb->bytes_per_sector == BYTES_PER_SECTOR &&
         "unsupported number of bytes per sector!");
}

void read_root_directory() {
  assert(bpb && "bios param block must be populated before root directory!");
  const auto fat_sectors = bpb->sectors_per_fat * bpb->num_of_fats;
  const auto root_directory_sector = bpb->num_of_reserved_sectors + fat_sectors;
  const auto root_directory_size =
      sizeof(directory_entry) * bpb->num_of_directory_entries;
  const auto root_directory_sector_count =
      root_directory_size / BYTES_PER_SECTOR;

  root_dir = alloc::array_of<directory_entry>(bpb->num_of_directory_entries);
  read_floppy_or_fail(root_dir, root_directory_sector,
                      root_directory_sector_count);
}

static const directory_entry *resolve_path(const char *path) {
  assert(root_dir && "can't resolve path before root dir is populated!");
  assert(bpb && "can't resolve path before bios param block is populated!");
  assert(path && path[0] == '/' && "path null or not rooted!");
  const char *rest = &path[1];

  char fragment[directory_entry::file_full_name_size + 1] = {0};
  auto read_next_fragment = [&](auto &rest) -> const char * {
    const char *dir_sep = strchr(rest, '/');
    const char *file_ext_sep = strchr(rest, '.');
    const char *end = dir_sep ? dir_sep : rest + strlen(rest);
    for (size_t i = 0; i < directory_entry::file_name_size; ++i) {
      if ((file_ext_sep && rest >= file_ext_sep) || (rest >= end))
        fragment[i] = ' ';
      else
        fragment[i] = toupper(*rest++);
    }
    assert((!file_ext_sep || (*rest == '.' && rest == file_ext_sep)) &&
           "did we skip over the dot?");
    if (file_ext_sep)
      ++rest;
    for (size_t i = directory_entry::file_name_size;
         i < directory_entry::file_full_name_size; ++i) {
      if (rest >= end)
        fragment[i] = ' ';
      else
        fragment[i] = toupper(*rest++);
    }
    fragment[directory_entry::file_full_name_size] = '\0';

    return fragment;
  };

  read_next_fragment(rest);

  const directory_entry *file_entry = nullptr;
  const directory_entry *directory = root_dir;
  const auto directory_size = bpb->num_of_directory_entries;
  for (auto i = 0; i < directory_size; ++i) {
    const auto &entry = directory[i];
    if (strncmp(entry.short_file_full_name, fragment,
                directory_entry::file_full_name_size) == 0) {
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
      fprintf(stderr,
              "searching for '%s'\n"
              "not found:    '%s'\n",
              fragment, entry.short_file_full_name);
    }
  }
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
      printf("/%s: size=%uB\n", file_name, entry.size);
      memset(file_name, 0, directory_entry::file_name_size);
    }
  }
}

kstd::unique_ptr<char> read_file(const char *path) {
  assert(bpb && "must have bios param block to read files!");
  assert(fat && "must have fat storage to read files!");
  const directory_entry *file_entry = resolve_path(path);
  if (!file_entry)
    return nullptr;

  fprintf(stderr, "fs: reading file: %s (%dB)", path, file_entry->size);

  auto buffer = alloc::array_of<char>(file_entry->size);
  const uint16_t first_fat_sector = bpb->num_of_reserved_sectors;
  const uint16_t num_of_fat_sectors = bpb->sectors_per_fat * bpb->num_of_fats;
  const uint16_t size_of_root_dir =
      bpb->num_of_directory_entries * sizeof(directory_entry);
  const auto num_of_root_dir_sectors =
      kstd::div_ceil(size_of_root_dir, (uint16_t)BYTES_PER_SECTOR);
  const auto first_data_sector =
      first_fat_sector + num_of_fat_sectors + num_of_root_dir_sectors;

  size_t offset = 0;
  uint16_t cluster = file_entry->cluster_num_lo;
  auto loaded_fat_sector = 0;
  while (cluster < 0xFF8) {
    const auto fat_offset = cluster + (cluster / 2);
    const auto fat_sector = first_fat_sector + (fat_offset / BYTES_PER_SECTOR);
    const auto entry_offset = fat_offset % BYTES_PER_SECTOR;
    if (fat_sector != loaded_fat_sector) {
      read_floppy_or_fail(fat, fat_sector, 2);
      loaded_fat_sector = fat_sector;
    }

    const uint16_t next_data_sector =
        ((cluster - 2) * bpb->sectors_per_cluster) + first_data_sector;

    read_floppy_or_fail(buffer + offset, next_data_sector, 1);
    offset += BYTES_PER_SECTOR;

    uint16_t fat_value = *(uint16_t *)(&fat[entry_offset]);
    cluster = (cluster & 1) ? fat_value >> 4 : fat_value & 0x0FFF;
  }

  return kstd::unique_ptr<char>(buffer);
}
} // namespace fs
