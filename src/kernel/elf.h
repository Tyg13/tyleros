#ifndef ELF_H
#define ELF_H

#include "libadt/range.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

namespace elf {

enum class section_flags : uint64_t {
  write = 0x1,
  alloc = 0x2,
  execinstr = 0x4,
};

struct section {
  uint32_t name;
  uint32_t type;
  section_flags flags;
  uint64_t addr;
  uint64_t offset;
  uint64_t size;
  uint32_t link;
  uint32_t info;
  uint64_t addralign;
  uint64_t entry_size;
} __attribute__((packed));
static_assert(sizeof(section) == 64);

struct header {
  uint8_t magic[4];
  uint8_t bitness;
  uint8_t endianness;
  uint8_t header_version;
  uint8_t os_abi;
  uint64_t : 64;
  uint16_t type;
  uint16_t machine;
  uint32_t elf_version;
  uint64_t entry_offset;
  uint64_t program_header_table_offset;
  uint64_t section_header_table_offset;
  uint32_t flags;
  uint16_t header_size;
  uint16_t size_of_program_header_table_entry;
  uint16_t num_of_program_header_table_entries;
  uint16_t size_of_section_header_table_entry;
  uint16_t num_of_section_header_table_entries;
  uint16_t section_index_to_section_header_string_table;

  section *sections_begin() {
    return (section *)((uintptr_t)this + section_header_table_offset);
  }
  const section *sections_begin() const {
    return (const section *)((uintptr_t)this + section_header_table_offset);
  }
  section *sections_end() {
    return sections_begin() + num_of_section_header_table_entries;
  }
  const section *sections_end() const {
    return sections_begin() + num_of_section_header_table_entries;
  }
  adt::range<section> sections() {
    return adt::range<section>(sections_begin(), sections_end());
  }
  adt::range<const section> sections() const {
    return adt::range<const section>(sections_begin(), sections_end());
  }
  size_t sections_size() const {
    return size_of_section_header_table_entry *
           num_of_section_header_table_entries;
  }
  const section *section_by_index(uint16_t index) const {
    assert(index < num_of_section_header_table_entries &&
           "section index out of range!");
    const auto addr = (uintptr_t)sections().begin() +
                      index * size_of_section_header_table_entry;
    const auto *ret = (const section *)addr;
    return ret->type == 0 ? nullptr : ret;
  }

  char *section_name_string_table() {
    const auto *string_section =
        section_by_index(section_index_to_section_header_string_table);
    const auto addr = (uintptr_t)this + string_section->offset;
    return string_section ? (char *)addr : nullptr;
  }
  const char *section_name_string_table() const {
    return (*const_cast<header *>(this)).section_name_string_table();
  }
  const char *section_name(const section &s) const {
    return section_name_string_table() + s.name;
  }

  size_t string_table_size() const {
    const auto *strings =
        section_by_index(section_index_to_section_header_string_table);
    return strings ? strings->size : 0;
  }
} __attribute__((packed));
static_assert(sizeof(header) == 64);

header &kernel_header();

void init();
} // namespace elf

#endif
