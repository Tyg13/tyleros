#include "idt.h"

#include "gdt.h"
#include "interrupts.h"
#include "pic.h"

#include <stdio.h>

namespace idt {

constexpr inline auto NUM_IDT_ENTRIES = 0x100;
using IDT = IDT_Entry[NUM_IDT_ENTRIES];

static IDTR g_idtr;
static IDT g_idt;

void init() {
  pic::remap_interrupts();

  for (auto i = 0; i < (int)NUM_IDT_ENTRIES; ++i) {
    const uintptr_t handler =
        interrupts::get_handler(static_cast<unsigned char>(i));
    g_idt[i] = {
        .offset_1 = (uint16_t)(handler),
        .selector = gdt::KERNEL_CODE_SELECTOR,
        .ist = 0,
        .type_attr = (uint8_t)IDT_Entry::Attr::present |
                     (uint8_t)IDT_Entry::Type::interrupt_32,
        .offset_2 = (uint16_t)(handler >> 16),
        .offset_3 = (uint32_t)(handler >> 32),
    };
  }
  g_idtr = {
      .limit = sizeof(g_idt) - 1,
      .base = reinterpret_cast<uintptr_t>(&g_idt),
  };

  asm volatile("lidt %0" ::"m"(g_idtr));
  printf("idt: initialized at 0x%p\n", &g_idt);
}

} // namespace idt
