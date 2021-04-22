#include "serial.h"

#include "io.h"

namespace Interrupts {
     constexpr auto NONE              = 0b0000;
     constexpr auto DATA_AVAILABLE    = 0b0001;
     constexpr auto TRANSMITTER_EMPTY = 0b0010;
     constexpr auto ERROR             = 0b0100;
     constexpr auto STATUS_CHANGE     = 0b1000;
};

namespace LineStatus {
     constexpr auto DATA_READY               = 0b0000'0001;
     constexpr auto OVERRUN_ERROR            = 0b0000'0010;
     constexpr auto PARITY_ERROR             = 0b0000'0100;
     constexpr auto FRAMING_ERROR            = 0b0000'1000;
     constexpr auto BREAK_INDICATOR          = 0b0001'0000;
     constexpr auto TRANSMITTER_BUFFER_EMPTY = 0b0010'0000;
     constexpr auto TRANSMITTER_EMPTY        = 0b0100'0000;
     constexpr auto IMPENDING_ERROR          = 0b1000'0000;
};

namespace LineControl {
     constexpr auto DATA_BITS_5  = 0b00;
     constexpr auto DATA_BITS_6  = 0b01;
     constexpr auto DATA_BITS_7  = 0b10;
     constexpr auto DATA_BITS_8  = 0b11;
     constexpr auto STOP_BITS_1  = 0b000;
     constexpr auto STOP_BITS_2  = 0b100;
     constexpr auto PARITY_NONE  = 0b000'000;
     constexpr auto PARITY_ODD   = 0b001'000;
     constexpr auto PARITY_EVEN  = 0b011'000;
     constexpr auto PARITY_MARK  = 0b101'000;
     constexpr auto PARITY_SPACE = 0b101'000;
     constexpr auto DATA_LATCH_ACCESS_BIT = 0b1000'0000;
};

namespace FIFOControl {
    constexpr auto ENABLE                     = 0b0000'0001;
    constexpr auto CLEAR_RECEIVE              = 0b0000'0010;
    constexpr auto CLEAR_TRANSMIT             = 0b0000'0100;
    constexpr auto INTERRUPT_TRIGGER_14_BYTES = 0b1100'0000;
}

namespace ModemControl {
    constexpr auto FORCE_DATA_READY   = 0b0000'0001;
    constexpr auto FORCE_REQUEST_SEND = 0b0000'0010;
    constexpr auto AUX_OUTPUT_1       = 0b0000'0100;
    constexpr auto AUX_OUTPUT_2       = 0b0000'1000;
}

constexpr auto DATA            = [](auto&& port) { return port + 0; };
constexpr auto BAUD_DIVISOR_LO = [](auto&& port) { return port + 0; };
constexpr auto BAUD_DIVISOR_HI = [](auto&& port) { return port + 1; };
constexpr auto INTERRUPTS      = [](auto&& port) { return port + 1; };
constexpr auto FIFO_CONTROL    = [](auto&& port) { return port + 2; };
constexpr auto LINE_CONTROL    = [](auto&& port) { return port + 3; };
constexpr auto MODEM_CONTROL   = [](auto&& port) { return port + 4; };
constexpr auto LINE_STATUS     = [](auto&& port) { return port + 5; };

namespace serial {

static void init_port(uint16_t port);
    
using port::COM1;

static bool s_initialized = false;
void init() {
    init_port(COM1);
    s_initialized = true;
}

bool initialized() { return s_initialized; }

void init_port(uint16_t port) {
    // Disable interrupts
    io::out(INTERRUPTS(port), Interrupts::NONE);

    // Latch bit controls access to baud divisor
    io::out(LINE_CONTROL(port), LineControl::DATA_LATCH_ACCESS_BIT);

    // baud rate is 115'200 ticks per second divided by the baud divisor
    io::out(BAUD_DIVISOR_LO(port), 3);
    io::out(BAUD_DIVISOR_HI(port), 0);

    // 8N1 (8 bits, no parity, 1 stop bit) is standard protocol
    // Both ends of the serial port must agree on this or transmission will get garbled
    io::out(LINE_CONTROL(port), LineControl::DATA_BITS_8 | LineControl::PARITY_NONE | LineControl::STOP_BITS_1);

    // Enable Serial FIFO (interrupt at 14 bytes, but we should never trigger that)
    io::out(
        FIFO_CONTROL(port),
        FIFOControl::INTERRUPT_TRIGGER_14_BYTES
            | FIFOControl::ENABLE
            | FIFOControl::CLEAR_TRANSMIT
            | FIFOControl::CLEAR_RECEIVE
    );

    // Override any error that may have occurred in the meantime
    io::out(
        MODEM_CONTROL(port),
        ModemControl::FORCE_DATA_READY
            | ModemControl::FORCE_REQUEST_SEND
            | ModemControl::AUX_OUTPUT_2
    );

    // Re-enable data interrupt (enabling access to the port)
    io::out(INTERRUPTS(port), Interrupts::DATA_AVAILABLE);
}

static bool transmit_ready(int16_t port) {
    return io::in(LINE_STATUS(port)) & LineStatus::TRANSMITTER_BUFFER_EMPTY;
}

void send(int16_t port, uint8_t b) {
    while (!transmit_ready(serial::port::COM1)) {}
    io::out(DATA(COM1), (uint8_t) b);
}

}
