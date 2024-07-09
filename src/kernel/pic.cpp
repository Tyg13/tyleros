#include "pic.h"

#include "util/io.h"

constexpr static uint8_t ICW1_ICW4 = 1 << 0;
constexpr static uint8_t ICW1_INIT = 1 << 4;

constexpr static uint8_t ICW4_8086 = 1 << 0;

void remap_pic() {
  // The master and slave PICs are configured by default to use IVT offsets
  // 0x08 to 0x0F and 0x70 to 0x7F, respectively. However, in protected mode,
  // these offsets clash with interrupts triggered by processor exceptions.
  // So this code initializes the PIC and remaps the controllers to usable
  // offsets

  const auto pic1_value = io::inb(PIC1_DATA);
  const auto pic2_value = io::inb(PIC2_DATA);

  // Init master and slave PIC with initialization control word (ICW)
  // Let it know we want to initialize it, and that we'll give it 4 ICWs total
  constexpr auto ICW1 = ICW1_INIT | ICW1_ICW4;
  io::outb(PIC1_COMMAND, ICW1);
  io::wait();
  io::outb(PIC2_COMMAND, ICW1);
  io::wait();

  // ICW2 is PIC IVT base offset.
  // Remap master PIC to 0x20
  // Remap slave PIC to 0x28
  io::outb(PIC1_DATA, 0x20);
  io::wait();
  io::outb(PIC2_DATA, 0x28);
  io::wait();

  // ICW3 is a bitfield representing which PIC to use for IRQ cascading
  // on x86. IRQ2 on the slave is connected to IR2 on the master,
  // IRQ2 on the ISA itself is routed to IR1 on the slave
  io::outb(PIC1_DATA, 1 << 2);
  io::wait();
  io::outb(PIC2_DATA, 1 << 1);
  io::wait();

  // ICW4 tells the PIC what mode to operate in.
  // 80x86 is the only valid mode for x86
  io::outb(PIC1_DATA, ICW4_8086);
  io::wait();
  io::outb(PIC2_DATA, ICW4_8086);
  io::wait();

  io::outb(PIC1_DATA, pic1_value);
  io::outb(PIC2_DATA, pic2_value);

  // Unmask IRQ2 -- otherwise slave IRQs (IRQ8 - IRQ15) won't get generated
  unmask_irq(irq::CASCADE);
}

void mask_irq(irq code) {
  const auto irq = (int)code;
  const auto port = irq < 8 ? PIC1_DATA : PIC2_DATA;
  const auto mask = irq < 8 ? (1 << irq) : (1 << (irq - 8));
  const auto value = io::inb(port) | mask;
  io::outb(port, value);
}

void unmask_irq(irq code) {
  const auto irq = (int)code;
  const auto port = irq < 8 ? PIC1_DATA : PIC2_DATA;
  const auto mask = irq < 8 ? (1 << irq) : (1 << (irq - 8));
  const auto value = io::inb(port) & ~mask;
  io::outb(port, value);
}

bool irq_is_masked(irq code) {
  const auto irq = (int)code;
  const auto port = irq < 8 ? PIC1_DATA : PIC2_DATA;
  const auto mask = irq < 8 ? (1 << irq) : (1 << (irq - 8));
  return (io::inb(port)&mask) != 0;
}
