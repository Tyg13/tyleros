#ifndef GDT_H
#define GDT_H

#include <stdint.h>

namespace gdt {
struct GDTR {
  uint16_t size;
  uintptr_t base;
} __attribute__((packed));

struct Entry {
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

static constexpr auto KERNEL_CODE_SELECTOR_IDX = 1;
static constexpr auto KERNEL_DATA_SELECTOR_IDX = 2;
static constexpr auto USER_CODE_SELECTOR_IDX = 3;
static constexpr auto USER_DATA_SELECTOR_IDX = 4;
static constexpr auto KERNEL_CODE_SELECTOR =
    KERNEL_CODE_SELECTOR_IDX * sizeof(Entry);
static constexpr auto KERNEL_DATA_SELECTOR =
    KERNEL_DATA_SELECTOR_IDX * sizeof(Entry);
static constexpr auto USER_CODE_SELECTOR =
    USER_CODE_SELECTOR_IDX * sizeof(Entry);
static constexpr auto USER_DATA_SELECTOR =
    USER_DATA_SELECTOR_IDX * sizeof(Entry);

void init();
} // namespace gdt

#endif
