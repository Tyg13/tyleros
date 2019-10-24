#include "pic.h"

#include "util/io.h"

constexpr static uint8_t ICW1_ICW4    = 1 << 0;
constexpr static uint8_t ICW1_INIT    = 1 << 4;

constexpr static uint8_t ICW4_8086    = 1 << 0;

void remap_pic() {
   // The master and slave PICs are configured by default to use IVT offsets
   // 0x08 to 0x0F and 0x70 to 0x7F, respectively. However, in protected mode,
   // these offsets clash with interrupts triggered by processor exceptions.
   // So this code initializes the PIC and remaps the controllers to usable offsets

   // Init master and slave PIC with initialization control word (ICW)
   // Let it know we want to initialize it, and that we'll give it 4 ICWs total
   constexpr auto ICW1 = ICW1_INIT | ICW1_ICW4;
   io::out(PIC1_COMMAND, ICW1); io::wait();
   io::out(PIC2_COMMAND, ICW1); io::wait();

   // ICW2 is PIC IVT base offset.
   // Remap master PIC to 0x20
   // Remap slave PIC to 0x80
   io::out(PIC1_DATA, 0x20); io::wait();
   io::out(PIC2_DATA, 0x28); io::wait();

   // ICW3 is a bitfield representing which PIC to use for IRQ cascading
   // on x86, IRQ2 connects the master to the slave
   // For some reason the slave gets an IRQ cascade of 1?
   io::out(PIC1_DATA, 1 << 2); io::wait();
   io::out(PIC2_DATA, 1 << 1); io::wait();

   // ICW4 tells the PIC what mode to operate in.
   // 80x86 is the only valid mode for x86
   io::out(PIC1_DATA, ICW4_8086); io::wait();
   io::out(PIC2_DATA, ICW4_8086); io::wait();
}
