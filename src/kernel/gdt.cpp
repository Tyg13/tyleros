#include "gdt.h"

#include "alloc.h"
#include "debug.h"
#include "memory.h"

namespace gdt {

GDTR gdtr;

alignas(0x4) Entry gdt[7];

constexpr static uint8_t GDT_PRESENT = 1 << 7;
constexpr static uint8_t GDT_KERNEL_SEGMENT = 0 << 5;
constexpr static uint8_t GDT_USER_SEGMENT = 3 << 5;
constexpr static uint8_t GDT_NOT_TSS = 1 << 4;
constexpr static uint8_t GDT_EXECUTABLE = 1 << 3;
constexpr static uint8_t GDT_DATA_GROWS_DOWN = 1 << 2;
constexpr static uint8_t GDT_CODE_CONFORMING = 1 << 2;
constexpr static uint8_t GDT_DATA_WRITABLE = 1 << 1;
constexpr static uint8_t GDT_CODE_READABLE = 1 << 1;
constexpr static uint8_t GDT_ACCESSED = 1 << 0;

constexpr static uint8_t TSS_TYPE_AVAILABLE = 0x9;
constexpr static uint8_t TSS_TYPE_BUSY = 0xB;

constexpr static uint8_t GDT_BYTE_GRANULARITY = 0 << 3;
constexpr static uint8_t GDT_PAGE_GRANULARITY = 1 << 3;
constexpr static uint8_t GDT_16_BIT = 0 << 2;
constexpr static uint8_t GDT_32_BIT = 1 << 2;
constexpr static uint8_t GDT_64_BIT = 1 << 1;

struct tss_entry_struct {
  uint32_t : 32;
  uint64_t rsp0;
  uint64_t rsp1;
  uint64_t rsp2;
  uint64_t : 64;
  uint64_t ist1;
  uint64_t ist2;
  uint64_t ist3;
  uint64_t ist4;
  uint64_t ist5;
  uint64_t ist6;
  uint64_t ist7;
  uint64_t : 64;
  uint16_t : 16;
  uint16_t iopb_offset;
} __attribute__((packed));

tss_entry_struct tss_entry;

static void init_tss();

void init() {
  // Null descriptor
  gdt[0] = Entry{{{
      .limit_lo = 0x0,
      .base_lo = 0x0,
      .base_hi_lo = 0x0,
      .access = 0x0,
      .limit_hi = 0x0,
      .flags = 0x0,
      .base_hi_hi = 0x0,
  }}};
  // Kernel code descriptor
  gdt[KERNEL_CODE_SELECTOR_IDX] = Entry{{{
      .limit_lo = 0xFFFF,
      .base_lo = 0x0,
      .base_hi_lo = 0x0,
      .access = GDT_PRESENT | GDT_KERNEL_SEGMENT | GDT_NOT_TSS |
                GDT_EXECUTABLE | GDT_CODE_READABLE,
      .limit_hi = 0xF,
      .flags = GDT_PAGE_GRANULARITY | GDT_64_BIT,
      .base_hi_hi = 0x0,
  }}};
  // Kernel data descriptor
  gdt[KERNEL_DATA_SELECTOR_IDX] = Entry{{{
      .limit_lo = 0xFFFF,
      .base_lo = 0x0,
      .base_hi_lo = 0x0,
      .access =
          GDT_PRESENT | GDT_KERNEL_SEGMENT | GDT_NOT_TSS | GDT_DATA_WRITABLE,
      .limit_hi = 0xF,
      .flags = GDT_PAGE_GRANULARITY | GDT_64_BIT,
      .base_hi_hi = 0x0,
  }}};
  // User-space code descriptor
  gdt[USER_CODE_SELECTOR_IDX] = Entry{{{
      .limit_lo = 0xFFFF,
      .base_lo = 0x0,
      .base_hi_lo = 0x0,
      .access = GDT_PRESENT | GDT_USER_SEGMENT | GDT_NOT_TSS | GDT_EXECUTABLE |
                GDT_CODE_READABLE,
      .limit_hi = 0xF,
      .flags = GDT_PAGE_GRANULARITY | GDT_64_BIT,
      .base_hi_hi = 0x0,
  }}};
  // User-space data descriptor
  gdt[USER_DATA_SELECTOR_IDX] = Entry{{{
      .limit_lo = 0xFFFF,
      .base_lo = 0x0,
      .base_hi_lo = 0x0,
      .access =
          GDT_PRESENT | GDT_USER_SEGMENT | GDT_NOT_TSS | GDT_DATA_WRITABLE,
      .limit_hi = 0xF,
      .flags = GDT_PAGE_GRANULARITY | GDT_64_BIT,
      .base_hi_hi = 0x0,
  }}};
  // Task State Segment
  gdt[5] = Entry{{{
      .limit_lo = (uint16_t)(sizeof(tss_entry) & 0xFFFF),
      .base_lo = (uint16_t)((uintptr_t)&tss_entry & 0xFFFF),
      .base_hi_lo = (uint8_t)(((uintptr_t)&tss_entry >> 16) & 0xFF),
      .access = GDT_PRESENT | TSS_TYPE_AVAILABLE,
      .limit_hi = (uint8_t)(sizeof(tss_entry) >> 16),
      .flags = GDT_64_BIT,
      .base_hi_hi = (uint8_t)(((uintptr_t)&tss_entry >> 24) & 0xFF),
  }}};
  gdt[6].tss_high = (uint32_t)((uintptr_t)&tss_entry >> 32);

  init_tss();

  gdtr = GDTR{
      .size = sizeof(gdt) - 1,
      .base = reinterpret_cast<uintptr_t>(&gdt),
  };

  asm volatile("" ::: "memory");
  asm volatile("lgdt %0" ::"m"(gdtr));
  asm volatile("mov %0, %%ax\t\r" ::"N"(5 * sizeof(Entry)));
  asm volatile("ltr %ax");

  printf("gdt: initialized at 0x%p\n", gdt);
}

alignas(
    memory::PAGE_ALIGN.val) static unsigned char task_stack[memory::PAGE_SIZE];

void init_tss() {
  tss_entry = {0};
  tss_entry.rsp0 = (uint64_t)(uintptr_t)&task_stack;
}

} // namespace gdt
