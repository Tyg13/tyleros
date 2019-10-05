#include "idt.h"
#include "interrupts.h"

IDTR g_idtr;

constexpr int IDT_Size = 0x100;
IDT_Entry idt[IDT_Size];

void load_idt() {
    const auto set_handler = [](auto& entry, const auto handler) {
        entry = {
            .offset_1  = (uint16_t)(handler),
            .selector  = 0x8,
            .ist       = 0,
            .type_attr = (uint8_t) IDT_Entry::Attr::present | (uint8_t) IDT_Entry::Type::interrupt_32,
            .offset_2  = (uint16_t)(handler >> 16),
            .offset_3  = (uint32_t)(handler >> 32),
        };
    };
    for (int i = 0; i < IDT_Size; ++i) {
        set_handler(idt[i], reinterpret_cast<uintptr_t>(&interrupt_handler));
    }
    g_idtr = {
        .limit = sizeof(idt) - 1,
        .base = (uint32_t) reinterpret_cast<uintptr_t>(&idt),
    };
    __asm__ ("lidt %0\n\t"
             "sti" :: "m"(g_idtr));
}
