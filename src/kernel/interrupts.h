#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stddef.h>

typedef size_t uword_t __attribute__((mode (__word__)));

struct interrupt_frame
{
    uword_t ip;
    uword_t cs;
    uword_t flags;
    uword_t sp;
    uword_t ss;
};

__attribute__((interrupt)) void interrupt_handler(interrupt_frame* frame);
__attribute__((interrupt)) void exception_handler(interrupt_frame* frame, size_t error_code);

#endif
