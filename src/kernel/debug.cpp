#include "debug.h"

#include "serial.h"
#include "vga.h"

#include <stdarg.h>
#include <stdio.h>

namespace debug {

bool puts(const char *str) {
  if (str == nullptr)
    return false;

  for (auto s = str; *s != '\0'; ++s)
    serial::send(serial::port::COM1, *s);
  serial::send(serial::port::COM1, '\n');

  if (vga::initialized)
    vga::string::puts(str);

  return true;
}

static constexpr auto WRITE_BUFFER_SIZE = 0x200;
static char write_buffer[WRITE_BUFFER_SIZE];
void printf(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  char *str = write_buffer;
  vsnprintf(str, WRITE_BUFFER_SIZE, fmt, args);

  va_end(args);

  for (auto s = str; *s != '\0'; ++s)
    serial::send(serial::port::COM1, *s);

  if (vga::initialized)
    vga::string::print(str);
}

} // namespace debug

void debug_print(const char *str) {
  if (str == nullptr)
    return;

  for (auto s = str; *s != '\0'; ++s)
    serial::send(serial::port::COM1, *s);

  if (vga::initialized)
    vga::string::print(str);
}
