#include "idt.h"

#include "interrupts.h"
#include "memory.h"
#include "pic.h"

namespace IDT {

    constexpr auto IDT_Size = 0x100;

    static IDTR g_idtr;
    static IDT_Entry idt[IDT_Size];

    static void load_idt();

    void init() {
       remap_pic();
       load_idt();
       unmask_irq(1);
       asm volatile ("sti" ::: "memory", "cc");
    }

    void load_idt() {
       for (auto i = 0; i < (int)IDT_Size; ++i) {
          const auto handler = reinterpret_cast<uintptr_t>(get_interrupt_handler(i));
          idt[i] = {
             .offset_1  = (uint16_t)(handler),
             .selector  = 0x8,
             .ist       = 0,
             .type_attr = (uint8_t) IDT_Entry::Attr::present | (uint8_t) IDT_Entry::Type::interrupt_32,
             .offset_2  = (uint16_t)(handler >> 16),
             .offset_3  = (uint32_t)(handler >> 32),
          };
       }
       g_idtr = {
          .limit = sizeof(idt) - 1,
          .base = reinterpret_cast<uintptr_t>(&idt),
       };

       asm volatile ("lidt %0\n\t" :: "m"(g_idtr));
    }
}
