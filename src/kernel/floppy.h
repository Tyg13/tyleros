#ifndef FLOPPY_H
#define FLOPPY_H

#include "memory.h"
#include <stdint.h>

extern volatile bool disk_interrupt_handled;

void init_floppy_driver(uint8_t drive_number);
void handle_floppy_interrupt();
void prepare_floppy_dma(void *buffer);

constexpr static auto FLOPPY_BUFFER_SIZE = 16 * memory::PAGE_SIZE;
constexpr static auto BYTES_PER_SECTOR = 512;

enum class floppy_status {
  ok,
  end_of_cylinder,
  data_error,
  overrun_underrun,
  no_data,
  not_writable,
  missing_address_mark,
  error_unknown,
};
[[nodiscard]] floppy_status read_floppy(void *buffer, int start_sector,
                                        int sector_count);
void read_floppy_or_fail(void *buffer, int start_sector, int sector_count);

#endif
