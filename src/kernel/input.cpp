#include "input.h"
#include "util/io.h"
#include "memory.h"
#include "mutex.h"
#include "pic.h"
#include "timing.h"

#include "libadt/ring_buffer.h"

namespace keyboard {
struct subscriber {
  handler *handle;
};

struct event_pump {
  adt::ring_buffer<event, 0x100> events;
  adt::ring_buffer<subscriber, 0x100> subscribers;
};

static kstd::managed_by_mutex<event_pump> main_event_pump;

// 0x4 * 0xB = 0x24 + 0x2 * 0xB = 0x3B
static constexpr char scancode_to_key[0x40] = {
    '\0', ' ', '1',  '2',  '3',  '4', '5', '6',  '7', '8', '9', '0',
    '-',  '=', '\b', '\t', 'q',  'w', 'e', 'r',  't', 'y', 'u', 'i',
    'o',  'p', '[',  ']',  '\n', 0x0, 'a', 's',  'd', 'f', 'g', 'h',
    'j',  'k', 'l',  ';',  '\'', '`', 0x1, '\\', 'z', 'x', 'c', 'v',
    'b',  'n', 'm',  ',',  '.',  '/', 0x2, '\0', 0x2, ' ',
};

constexpr static io::port<io::read> PS_2_DATA{0x60};
constexpr static io::port<io::read> PS_2_CONTROL{0x64};

static bool ready_to_read() { return (io::inb(PS_2_CONTROL) & 1) == 1; }

void handle_interrupt() {
  while (ready_to_read()) {
    const auto scancode = io::inb(PS_2_DATA);
    if (scancode == 0x00) {
      // Key detection error or internal buffer overrun
      return;
    }

    const bool pressed = (scancode & 0x80) == 0;
    const auto scancode_offset = pressed ? scancode : (scancode & ~0x80);
    if (scancode_offset < 0x40) {
      if (auto pump = main_event_pump.try_lock()) {
        const char key = scancode_to_key[scancode_offset];
        (*pump)->events.push_back(event{key, pressed});
      }
    }
  }
}

void subscribe(handler h) {
  main_event_pump.lock()->subscribers.push_back(subscriber{h});
}

void dispatch_key_events(void *) {
  for (;;) {
    if (auto pump = main_event_pump.try_lock()) {
      adt::optional<event> e;
      while ((e = (*pump)->events.pop_front()))
        for (auto &subscriber : (*pump)->subscribers)
          subscriber.handle(*e);
    }
    scheduler::yield();
  }
}

void init() {
  scheduler::schedule_kernel_task(dispatch_key_events, nullptr);
  pic::unmask_irq(irq::KEYBOARD);
}
} // namespace keyboard
