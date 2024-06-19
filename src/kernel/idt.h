#ifndef IDT_H
#define IDT_H

#include <stdint.h>

namespace idt {

struct IDTR {
  uint16_t limit;
  uintptr_t base;
} __attribute((packed));

struct IDT_Entry {
  uint16_t offset_1;
  uint16_t selector;
  uint8_t ist;
  uint8_t type_attr;
  uint16_t offset_2;
  uint32_t offset_3;
  uint32_t zero = 0;

  enum class Type : uint8_t {
    task_32 = 0x5,
    interrupt_16 = 0x6,
    trap_16 = 0x7,
    interrupt_32 = 0xE,
    trap_32 = 0xF,
  };
  enum class Attr : uint8_t { present = 0x80 };
} __attribute((packed));

void init();

inline IDTR get_current_idtr() {
  IDTR idtr;
  asm volatile("sidt %0" ::"m"(idtr));
  return idtr;
}

} // namespace idt

#endif
