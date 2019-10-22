#include "pic.h"

#include "util/io.h"

constexpr static uint8_t PIC1_COMMAND = 0x20;
constexpr static uint8_t PIC1_DATA    = 0x21;
constexpr static uint8_t PIC2_COMMAND = 0xA0;
constexpr static uint8_t PIC2_DATA    = 0xA2;

constexpr static uint8_t ICW1_ICW4    = 1 << 0;
constexpr static uint8_t ICW1_INIT    = 1 << 4;

constexpr static uint8_t ICW4_8086    = 1 << 0;

void remap_pic() {
   // Init master and slave PIC with initialization control word (ICW)
   // Let it know we want to initialize it, and that we'll give it 4 ICWs total
   constexpr auto ICW1 = ICW1_INIT | ICW1_ICW4;
   io::out<uint8_t>(PIC1_COMMAND, ICW1, io::wait{});
   io::out<uint8_t>(PIC2_COMMAND, ICW1, io::wait{});

   // ICW2 is PIC IVT base offset.
   // Remap master PIC to 0x20
   // Remap slave PIC to 0x80
   io::out<uint8_t>(PIC1_DATA, 0x20, io::wait{});
   io::out<uint8_t>(PIC2_DATA, 0x28, io::wait{});

   // ICW3 is a bitfield representing which PIC to use for IRQ cascading
   // on x86, IRQ2 connects the master to the slave
   // For some reason the slave gets an IRQ cascase of 1?
   io::out<uint8_t>(PIC1_DATA, 1 << 2, io::wait{});
   io::out<uint8_t>(PIC2_DATA, 1 << 1, io::wait{});

   // ICW4 tells the PIC what mode to operate in.
   // 80x86 is the only valid mode for x86
   io::out<uint8_t>(PIC1_DATA, ICW4_8086, io::wait{});
   io::out<uint8_t>(PIC2_DATA, ICW4_8086, io::wait{});

   // Unmask all the PIC interrupts
   io::out<uint8_t>(PIC1_DATA, 0);
   io::out<uint8_t>(PIC2_DATA, 0);
}
