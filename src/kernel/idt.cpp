#include "idt.h"

#include "interrupts.h"
#include "memory.h"
#include "pic.h"
#include "pit.h"

IDTR g_idtr;

constexpr int IDT_Size = 0x100;

alignas(0x1000)
IDT_Entry idt[IDT_Size];

static void load_idt();

void init_interrupts() {
   remap_pic();
   load_idt();
   init_pit();
}

void load_idt() {
    for (unsigned int i = 0; i < IDT_Size; ++i) {
       auto handler = reinterpret_cast<uintptr_t>(get_interrupt_handler(i));
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
    asm volatile ("lidt %0\n\t"
                  "sti" :: "m"(g_idtr));
}
