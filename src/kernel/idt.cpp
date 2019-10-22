#include "idt.h"

#include "interrupts.h"
#include "pic.h"

IDTR g_idtr;

constexpr int IDT_Size = 0x100;
IDT_Entry idt[IDT_Size];

static void load_idt();

void init_interrupts() {
   load_idt();
   remap_pic();
}

void load_idt() {
    for (int i = 0; i < IDT_Size; ++i) {
       auto handler = reinterpret_cast<uintptr_t>(&interrupt_handler);
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
       .base = (uint32_t) reinterpret_cast<uintptr_t>(&idt),
    };
    asm volatile ("lidt %0\n\t"
                  "sti" :: "m"(g_idtr));
}
