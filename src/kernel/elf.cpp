#include "elf.h"
#include "filesystem.h"
#include "memory.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

namespace elf {

static header *KERNEL_ELF_HEADER;

header &kernel_header() {
  return *KERNEL_ELF_HEADER;
}

void init() {
  //KERNEL_ELF_HEADER =
  //    reinterpret_cast<header *>(fs::read_file("/kernel.sym").release());
  //assert(strncmp((char *)KERNEL_ELF_HEADER->magic, "\177ELF", 4) == 0 &&
  //       "loading kernel symbol tables failed!");
  //assert(KERNEL_ELF_HEADER->size_of_section_header_table_entry ==
  //           sizeof(section) &&
  //       "ELF section size mismatch!");

  //printf("ELF: loaded header   at 0x%p\n", &KERNEL_ELF_HEADER);
  //printf("ELF: loaded sections at 0x%p - 0x%p\n",
  //       KERNEL_ELF_HEADER->sections().begin(),
  //       KERNEL_ELF_HEADER->sections().end());
  //printf("ELF: loaded strings  at 0x%p - 0x%zx\n",
  //       KERNEL_ELF_HEADER->section_name_string_table(),
  //       (uintptr_t)KERNEL_ELF_HEADER->section_name_string_table() +
  //           KERNEL_ELF_HEADER->string_table_size());
  //for (const section &s : KERNEL_ELF_HEADER->sections()) {
  //  printf("ELF: section '%s' at %p\n", KERNEL_ELF_HEADER->section_name(s),
  //         (char *)memory::KERNEL_VMA_START + s.offset);
  //}
}
} // namespace elf
