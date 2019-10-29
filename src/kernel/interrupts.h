#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stddef.h>
#include <stdint.h>

typedef size_t uword_t __attribute__((mode (__word__)));

struct interrupt_frame
{
    uword_t ip;
    uword_t cs;
    uword_t flags;
    uword_t sp;
    uword_t ss;
};

uintptr_t get_interrupt_handler(unsigned int vector_index);

#endif
