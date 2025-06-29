#ifndef CMOS_H
#define CMOS_H

#include <stdint.h>

namespace cmos {
void init();
bool initialized();

uint64_t get_ticks_since_boot();
} // namespace cmos

#endif
