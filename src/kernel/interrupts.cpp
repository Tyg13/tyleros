#include "interrupts.h"

extern void * after_exception;

void interrupt_handler(interrupt_frame* frame) { }

void exception_handler(interrupt_frame* frame, size_t error_code) { }

