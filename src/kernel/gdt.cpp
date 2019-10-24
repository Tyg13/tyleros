#include "gdt.h"

#include "memory.h"

GDTR gdtr;

alignas(0x4)
GDT_Entry gdt[3];

constexpr static uint8_t GDT_PRESENT         = 1 << 7;
constexpr static uint8_t GDT_KERNEL_SEGMENT  = 0 << 5;
constexpr static uint8_t GDT_USER_SEGMENT    = 3 << 5;
constexpr static uint8_t GDT_NOT_TSS         = 1 << 4;
constexpr static uint8_t GDT_EXECUTABLE      = 1 << 3;
constexpr static uint8_t GDT_DATA_GROWS_DOWN = 1 << 2;
constexpr static uint8_t GDT_CODE_CONFORMING = 1 << 2;
constexpr static uint8_t GDT_DATA_WRITABLE   = 1 << 1;
constexpr static uint8_t GDT_CODE_READABLE   = 1 << 1;

constexpr static uint8_t GDT_BYTE_GRANULARITY = 0 << 3;
constexpr static uint8_t GDT_PAGE_GRANULARITY = 1 << 3;
constexpr static uint8_t GDT_16_BIT           = 0 << 2;
constexpr static uint8_t GDT_32_BIT           = 1 << 2;
constexpr static uint8_t GDT_64_BIT           = 1 << 1;

void init_gdt() {
   // Null descriptor
   gdt[0] = GDT_Entry {
      .limit_lo   = 0x0,
      .base_lo    = 0x0,
      .base_hi_lo = 0x0,
      .access     = 0x0,
      .limit_hi   = 0x0,
      .flags      = 0x0,
      .base_hi_hi = 0x0,
   };
   // Kernel code descriptor
   gdt[1] = GDT_Entry {
      .limit_lo   = 0xFFFF,
      .base_lo    = 0x0,
      .base_hi_lo = 0x0,
      .access     = GDT_PRESENT
                  | GDT_KERNEL_SEGMENT
                  | GDT_NOT_TSS
                  | GDT_EXECUTABLE
                  | GDT_CODE_READABLE,
      .limit_hi   = 0xF,
      .flags      = GDT_PAGE_GRANULARITY | GDT_64_BIT,
      .base_hi_hi = 0x0,
   };
   // Kernel data descriptor
   gdt[2] = GDT_Entry {
      .limit_lo   = 0xFFFF,
      .base_lo    = 0x0,
      .base_hi_lo = 0x0,
      .access     = GDT_PRESENT
                  | GDT_KERNEL_SEGMENT
                  | GDT_NOT_TSS
                  | GDT_DATA_WRITABLE,
      .limit_hi   = 0xF,
      .flags      = GDT_PAGE_GRANULARITY | GDT_64_BIT,
      .base_hi_hi = 0x0,
   };

   gdtr = GDTR {
      .size = sizeof(gdt) - 1,
      .base = reinterpret_cast<uintptr_t>(&gdt) - KERNEL_VMA_OFFSET,
   };

   GDTR * gdtr_linear = reinterpret_cast<GDTR *>(reinterpret_cast<uintptr_t>(&gdtr) - KERNEL_VMA_OFFSET);

   asm volatile ("lgdt %0" :: "m"(*gdtr_linear));
}
