#ifndef PIC_H
#define PIC_H

#include "util/io.h"

constexpr static uint8_t PIC1_COMMAND = 0x20;
constexpr static uint8_t PIC1_DATA = 0x21;
constexpr static uint8_t PIC2_COMMAND = 0xA0;
constexpr static uint8_t PIC2_DATA = 0xA1;

constexpr static uint8_t END_OF_INTERRUPT = 0x20;

void remap_pic();

enum class irq: int {
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

void mask_irq(irq code);
void unmask_irq(irq code);

bool irq_is_masked(irq code);

#endif
