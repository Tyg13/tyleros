#ifndef GDT_H
#define GDT_H

#include <stdint.h>

namespace gdt {
struct GDTR {
  uint16_t size;
  uintptr_t base;
} __attribute__((packed));

struct GDT_Entry {
  union {
    struct {
      uint16_t limit_lo;
      uint16_t base_lo;
      uint8_t base_hi_lo;
      uint8_t access;
      unsigned limit_hi : 4;
      unsigned flags : 4;
      uint8_t base_hi_hi;
    };
    uint32_t tss_high;
  };
} __attribute__((packed));

void init();
} // namespace gdt

#endif
