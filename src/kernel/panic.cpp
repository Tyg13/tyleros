#include "panic.h"

namespace kstd {
void panic(const char *msg, ...) {
  va_list args;
  va_start(args, msg);
  vfprintf(stdout, msg, args);
  va_end(args);
  asm volatile("hlt");
loop:
  goto loop;
}
}
