#ifndef KERNEL_INPUT_H
#define KERNEL_INPUT_H

namespace keyboard {
void init();

enum event_type {
  key
};
struct event {
  char code;
  bool pressed;
};

using handler = void(event);
void subscribe(handler s);

void handle_interrupt();
} // namespace keyboard

#endif
