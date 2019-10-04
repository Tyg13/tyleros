#include "idt.h"

struct interrupt_frame
{
    size_t ip;
    size_t cs;
    size_t flags;
    size_t sp;
    size_t ss;
} __attribute((packed));

__attribute__((interrupt))
void interrupt_handler(interrupt_frame* frame) {
    ++frame->ip;
}

struct IDTR {
    uint16_t limit;
    uint32_t base;
} __attribute((packed)) g_idtr;

struct IDT_Entry {
    uint16_t offset_1;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_2;
    uint32_t offset_3;
    uint32_t zero = 0;

    enum Type : uint8_t {
        task_32      = 0x5,
        interrupt_16 = 0x6,
        trap_16      = 0x7,
        interrupt_32 = 0xE,
        trap_32      = 0xF,
    };
    enum Attr : uint8_t {
        present = 1 << 7
    };
} __attribute((packed));

constexpr int IDT_Size = 0x100;
IDT_Entry idt[IDT_Size];

void load_idt() {
    for (int i = 0; i < IDT_Size; ++i) {
        idt[i] = {
            .offset_1 = (uint16_t)(size_t)(&interrupt_handler),
            .selector = 0x8,
            .ist = 0,
            .type_attr = IDT_Entry::present | IDT_Entry::trap_32,
            .offset_2 = (uint16_t)((size_t)(&interrupt_handler) >> 16),
            .offset_3 = (uint32_t)((size_t)(&interrupt_handler) >> 32),
        };
    }
    g_idtr = {
        .limit = sizeof(idt) - 1,
        .base = (uint32_t)(size_t)&idt,
    };
    __asm__ ("lidt %0\n\t"
             "sti" :: "m"(g_idtr));
}
