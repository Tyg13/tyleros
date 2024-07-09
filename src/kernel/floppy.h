#ifndef FLOPPY_H
#define FLOPPY_H

#include <stdint.h>

extern volatile bool disk_interrupt_handled;

void init_floppy_driver(uint8_t drive_number);
void prepare_floppy_dma(void *buffer);

enum class floppy_status {
  ok,
  too_few_sectors,
  driver_too_slow,
  media_write_protected,
  error_unknown,
};
[[nodiscard]] floppy_status read_floppy(void *buffer, int start_sector,
                                        int sector_count);
void read_floppy_or_fail(void *buffer, int start_sector, int sector_count);

#endif
