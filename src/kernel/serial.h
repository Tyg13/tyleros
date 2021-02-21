#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

namespace serial {
    namespace port {
        constexpr auto COM1 = 0x3F8;
    }
    void init();

    void send(int16_t port, uint8_t b);
}

#endif
