#include "acpi.h"
#include "paging.h"

#include <string.h>

namespace acpi {
using segmented_ptr = const char[2];
void *convert_to_linear_ptr(segmented_ptr p) {
  return (void *)((uintptr_t)p[1] + (uintptr_t)p[0] * 0x10);
}

static RSDP *rsdp = nullptr;

bool RSDP::valid() const {
  const auto *in = reinterpret_cast<const unsigned char *>(this);
  unsigned sum = 0;
  for (int i = 0; i < (int)sizeof(RSDP); ++i)
    sum += in[i];
  return static_cast<unsigned char>(sum) == 0;
}

void init() {
  constexpr static uintptr_t rsdp_area_start = 0x000E0000;
  constexpr static uintptr_t rsdp_area_end = 0x00100000;
  constexpr static ptrdiff_t rsdp_area_size = rsdp_area_end - rsdp_area_start;

  paging::kernel_page_tables.identity_map_page_range_into_kernel_space(
      rsdp_area_start, rsdp_area_size);

  // Find RDSP in the first 1KB of the EBDA, maybe?
  rsdp = reinterpret_cast<RSDP *>(
      memstr((void *)rsdp_area_start, "RSD PTR ", rsdp_area_size));

  if (rsdp) {
    fprintf(stderr, "rsdp: %p\n", rsdp);
    fprintf(stderr, "\tchecksum: %d\n", rsdp->checksum);
    char buffer[17];
    strncpy(buffer, rsdp->oem_id, 16);
    fprintf(stderr, "\toem: %s\n", buffer);
    fprintf(stderr, "\trevision: %d\n", rsdp->revision);
    fprintf(stderr, "\trsdt addr: 0x%p\n", (void *)(uintptr_t)rsdp->rsdt_address);
    fprintf(stderr, "\tvalid: %s\n", (rsdp->valid() ? "yes" : "no"));
  } else
    printf("rsdp: not found!\n");
}
} // namespace acpi
