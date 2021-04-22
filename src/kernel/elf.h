#ifndef ELF_H
#define ELF_H

#include <stdint.h>

namespace elf {

struct elf_header {
    uint8_t MAGIC[0x4];
    uint8_t bitness;
    uint8_t endianness;
    uint8_t header_version;
    uint8_t abi;
    uint16_t : 16;
    uint8_t type           : 2;
    uint8_t architecture   : 1;
    uint8_t elf_version    : 4;
    uintptr_t entry_offset;
    uintptr_t program_header_table_offset;
    uintptr_t section_header_table_offset;
    uint32_t flags;
} __attribute__((packed));

}

#endif
