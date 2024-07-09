#include "elf.h"

#include "debug.h"
#include "low_memory_allocator.h"

#include <string.h>

namespace elf {

static header *KERNEL_ELF_HEADER = nullptr;

header &kernel_header() {
  return *KERNEL_ELF_HEADER;
}

void init(uintptr_t kernel_physical_start, uintptr_t kernel_physical_end) {
  const header &in_header = *(const elf::header *)kernel_physical_start;
  header *kernel = KERNEL_ELF_HEADER;
  kernel = low_memory::allocate<header>();
  *kernel = in_header;

  // faults if low memory containing the kernel physical start/end are unmapped
  memcpy(kernel->sections(), in_header.sections(), in_header.sections_size());
  memcpy(kernel->section_name_string_table(),
         in_header.section_name_string_table(), in_header.string_table_size());
  debug::printf("ELF: loaded header   at 0x%p\n", &kernel);
  debug::printf("ELF: loaded sections at 0x%p - 0x%zx\n", kernel->sections(),
                (uintptr_t)kernel->sections() + kernel->sections_size());
  debug::printf("ELF: loaded strings  at 0x%p - 0x%zx\n",
                kernel->section_name_string_table(),
                (uintptr_t)kernel->section_name_string_table() +
                    kernel->string_table_size());
}
} // namespace elf
