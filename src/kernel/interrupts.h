#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stddef.h>
#include <stdint.h>

struct interrupt_frame {
  uint64_t rip;
  uint64_t cs;
  uint64_t rflags;
  uint64_t rsp;
  uint64_t ss;
} __attribute__((packed));
static_assert(sizeof(interrupt_frame) == 40);

uintptr_t get_interrupt_handler(unsigned int vector_index);

#endif
