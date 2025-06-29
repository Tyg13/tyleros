#ifndef PANIC_H
#define PANIC_H

#include <stdarg.h>
#include <stdio.h>

namespace kstd {
[[noreturn]] __attribute__((cold)) void panic(const char *msg, ...);
}

#endif // PANIC_H
