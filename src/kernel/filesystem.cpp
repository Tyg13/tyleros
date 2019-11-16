#include "filesystem.h"

#include "floppy.h"

#include <stdint.h>

struct bios_parameter_block {
   uint8_t  jumpcode  [3];
   char     identifier[8];
   uint16_t bytes_per_sector;
   uint8_t  sectors_per_cluster;
   uint16_t num_of_reserved_sectors;
   uint8_t  num_of_fats;
   uint16_t num_of_directory_entries;
   uint16_t small_sector_count;
   uint8_t  media_descriptor_type;
   uint16_t sectors_per_fat;
   uint16_t sectors_per_track;
   uint16_t num_of_heads;
   uint32_t num_of_hidden_sectors;
   uint32_t large_sector_count;
   struct {
      uint8_t  drive_number;
      uint8_t  reserved;
      uint8_t  signature;
      uint32_t serial_number;
      char     volume_label     [11];
      char     system_identifier[8];
   } __attribute__((packed));
} __attribute__((packed));
static_assert(sizeof(bios_parameter_block) == 62);

struct directory_entry {
   char short_file_name[11];
   enum : uint8_t {
      READ_ONLY      = 0x01,
      HIDDEN         = 0x02,
      SYSTEM         = 0x04,
      VOLUME_ID      = 0x08,
      DIRECTORY      = 0x10,
      ARCHIVE        = 0x20,
      LONG_FILE_NAME = READ_ONLY | HIDDEN | SYSTEM | VOLUME_ID,
   } attributes;
   uint8_t reserved;
   uint8_t creation_time_deciseconds;
   uint16_t creation_hour        : 5;
   uint16_t creation_minutes     : 6;
   uint16_t creation_halfseconds : 5;
   uint16_t creation_year        : 7;
   uint16_t creation_month       : 4;
   uint16_t creation_day         : 5;
   uint16_t accessed_year        : 7;
   uint16_t accessed_month       : 4;
   uint16_t accessed_day         : 5;
   uint16_t cluster_num_hi;
   uint16_t modified_hour        : 5;
   uint16_t modified_minutes     : 6;
   uint16_t modified_halfseconds : 5;
   uint16_t modified_year        : 7;
   uint16_t modified_month       : 4;
   uint16_t modified_day         : 5;
   uint16_t cluster_num_lo;
   uint32_t size;
} __attribute__((packed));
static_assert(sizeof(directory_entry) == 32);

bios_parameter_block bpb;
directory_entry * root_dir;

static void read_boot_sector();
static void read_root_directory();

void init_filesystem() {
   read_boot_sector();
   read_root_directory();
}

void read_boot_sector() {
   read_floppy_sector(0);
   bpb = *reinterpret_cast<const bios_parameter_block *>(floppy_dma_buffer);
}

void read_root_directory() {
   const auto root_directory_sector = bpb.num_of_reserved_sectors
                                    + bpb.sectors_per_fat * bpb.num_of_fats;
   root_dir = new directory_entry[bpb.num_of_directory_entries];
   read_floppy_sector(root_directory_sector);
   for (int i = 0; i < bpb.num_of_directory_entries; ++i) {
      const auto entry_ptr = reinterpret_cast<const directory_entry *>(floppy_dma_buffer) + i;
      root_dir[i] = *entry_ptr;
   }
}
