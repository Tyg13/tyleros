#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stddef.h>
#include <stdint.h>

typedef size_t uword_t __attribute__((mode (__word__)));

struct interrupt_frame
{
    uword_t rip;
    uword_t cs;
    uword_t rflags;
    uword_t rsp;
    uword_t ss;
} __attribute__((packed));
static_assert(sizeof(interrupt_frame) == 40);

uintptr_t get_interrupt_handler(unsigned int vector_index);

#endif
