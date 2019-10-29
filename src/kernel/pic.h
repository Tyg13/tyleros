#ifndef PIC_H
#define PIC_H

#include "util/io.h"

constexpr static uint8_t PIC1_COMMAND = 0x20;
constexpr static uint8_t PIC1_DATA    = 0x21;
constexpr static uint8_t PIC2_COMMAND = 0xA0;
constexpr static uint8_t PIC2_DATA    = 0xA1;

constexpr static uint8_t END_OF_INTERRUPT = 0x20;

void remap_pic();

#endif
