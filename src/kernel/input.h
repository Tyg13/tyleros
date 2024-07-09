#ifndef KERNEL_INPUT_H
#define KERNEL_INPUT_H

#include "mutex.h"
#include "adt/ring_buffer.h"

namespace keyboard {
void init();

constexpr static auto BUFFER_CAPACITY = 512;
extern kstd::managed_by_mutex<kstd::ring_buffer<char, 512>> buffer;

void handle_interrupt();
} // namespace keyboard

#endif
