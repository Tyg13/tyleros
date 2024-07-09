#ifndef DEBUG_H
#define DEBUG_H

namespace debug {
bool puts(const char *str);
void printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
} // namespace debug

extern "C" void debug_print(const char *str);

#endif
