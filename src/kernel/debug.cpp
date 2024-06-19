#include "debug.h"

#include "io.h"
#include "serial.h"
#include "vga.h"

#include <stdarg.h>
#include <stdio.h>

namespace debug {

static bool s_enabled = false;
bool enabled() { return s_enabled; }

bool try_to_enable() {
  if (!serial::initialized()) {
    return false;
  }
  s_enabled = true;
  return true;
}

bool puts(const char *str) {
  if (str == nullptr)
    return false;

  for (auto c = *str; c != '\0'; c = *++str) {
    serial::send(serial::port::COM1, c);
  }

  if (vga::initialized)
    vga::string::puts(str);

  return true;
}

static constexpr auto WRITE_BUFFER_SIZE = 0x200;
static char write_buffer[WRITE_BUFFER_SIZE];
void printf(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  vsnprintf(write_buffer, WRITE_BUFFER_SIZE, fmt, args);

  va_end(args);

  puts(write_buffer);
}

} // namespace debug
