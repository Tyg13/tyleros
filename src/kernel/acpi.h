#ifndef KERNEL_ACPI_H
#define KERNEL_ACPI_H

#include <stdint.h>

namespace acpi {

struct RSDP {
  char signature[8];
  uint8_t checksum;
  char oem_id[16];
  uint8_t revision;
  uint32_t rsdt_address;

  bool valid() const;
} __attribute__((packed));

struct FADT {

};

void init();
} // namespace acpi

#endif
