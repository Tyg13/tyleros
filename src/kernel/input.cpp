#include "input.h"
#include "io.h"
#include "pic.h"

namespace keyboard {
kstd::managed_by_mutex<kstd::ring_buffer<char, BUFFER_CAPACITY>> buffer;

static constexpr char scancode_to_key[0x40] = {
    '\0', ' ', '1',  '2',  '3',  '4', '5', '6',  '7', '8', '9', '0',
    '-',  '=', '\b', '\t', 'q',  'w', 'e', 'r',  't', 'y', 'u', 'i',
    'o',  'p', '[',  ']',  '\n', 0x0, 'a', 's',  'd', 'f', 'g', 'h',
    'j',  'k', 'l',  ';',  '\'', '`', 0x1, '\\', 'z', 'x', 'c', 'v',
    'b',  'n', 'm',  ',',  '.',  '/', 0x2, '\0', 0x2, ' ',
};

void handle_interrupt() {
  constexpr static auto PS_2_DATA = 0x60;
  const auto scancode = io::inb(PS_2_DATA);
  if (scancode < 0x40) {
    if (auto buffer = keyboard::buffer.try_lock())
      (*buffer)->push_back(scancode_to_key[scancode]);
  }
}

void init() { unmask_irq(irq::KEYBOARD); }
} // namespace keyboard
