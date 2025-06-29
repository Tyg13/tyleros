#ifndef PIC_H
#define PIC_H

enum class irq : int {
  PIT = 0,
  KEYBOARD = 1,
  CASCADE = 2,
  COM2 = 3,
  COM1 = 4,
  LPT2 = 5,
  FLOPPY = 6,
  LPT1 = 7,
  CMOS_RTC = 8,
  PERIPHERAL_1 = 9,
  PERIPHERAL_2 = 10,
  PERIPHERAL_3 = 11,
  PS2_MOUSE = 12,
  FPU = 13,
  PRIMARY_ATA = 14,
  SECONDARY_ATA = 15,
};

namespace pic {

void remap_interrupts();

void mask_irq(irq code);
void unmask_irq(irq code);
bool irq_is_masked(irq code);

void signal_end_of_interrupt(irq code);
} // namespace pic

#endif
