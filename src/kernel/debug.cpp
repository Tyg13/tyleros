#include "debug.h"

#include "serial.h"
#include "io.h"

#include <stdio.h>
#include <stdarg.h>

namespace debug {

bool write_str(const char * str) {
    if (str == nullptr) return false;

    for (auto c = *str; c != '\0'; c = *++str) {
        serial::send(serial::port::COM1, c);
    }
    return true;
}

static char write_buffer[0x200];
void printf(const char * fmt, ...) {
    va_list args;
    va_start(args, fmt);

    vsprintf(write_buffer, fmt, args);

    va_end(args);

    write_str(write_buffer);
}

}
