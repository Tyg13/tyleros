#include "vga.h"

#include "mutex.h"
#include "paging.h"

#include <stdint.h>

namespace vga {

const cursor null_cursor = {.x = -1, .y = -1};
static cursor current_cursor = {.x = 0, .y = 0};

static void write_color_char_at(const char c, color fg, color bg, cursor pos);

const auto advance_x = [](auto &cursor) {
  if (++cursor.x >= SCREEN_WIDTH)
    cursor.x = 0;
};
const auto advance_y_if_needed = [](auto &cursor) {
  // If wrapping occurred
  if (cursor.x == 0) {
    if (++cursor.y >= SCREEN_HEIGHT) {
      cursor.y = 0;
    }
  }
};
const auto advance = [](auto &cursor) {
  advance_x(cursor);
  advance_y_if_needed(cursor);
};

static mutex write_lock;

void string::write() const {
  const auto lock = mutex_guard{write_lock};

  if (position != null_cursor) {
    current_cursor = position;
  }
  auto *cursor = text;
  while (*cursor != '\0') {
    char c = *cursor;
    if (c == '\n') {
      current_cursor.x = 0;
      advance_y_if_needed(current_cursor);
    } else if (c == '\b') {
      if (--current_cursor.x < 0) {
        current_cursor.x = 0;
      }
      write_color_char_at(' ', color::black, color::black, current_cursor);
    } else {
      write_color_char_at(c, foreground, background, current_cursor);
      advance(current_cursor);
    }
    ++cursor;
  }
}

constexpr auto VGA_MEMORY_BASE_ADDRESS = 0xB8000;

static void write_color_char_at(const char c, color fg, color bg, cursor pos) {
  auto vga_memory_base =
      reinterpret_cast<volatile uint16_t *>(VGA_MEMORY_BASE_ADDRESS);
  auto *screen_pos = vga_memory_base + pos.x + pos.y * SCREEN_WIDTH;
  auto color = (uint16_t)((unsigned char)bg << 4 | (unsigned char)fg);
  *screen_pos = color << 8 | (uint16_t)c;
}

bool initialized = false;
void init() {
  // Identity map the entire VGA buffer.
  map_range_size(VGA_MEMORY_BASE_ADDRESS, VGA_MEMORY_BASE_ADDRESS,
                 SCREEN_HEIGHT * SCREEN_WIDTH * sizeof(uint16_t));
  clear_screen();
  initialized = true;
}

void clear_screen() {
  auto c = cursor{.x = 0, .y = 0};
  for (int i = 0; i < SCREEN_HEIGHT; ++i) {
    for (int j = 0; j < SCREEN_WIDTH; ++j) {
      write_color_char_at(' ', color::black, color::black, c);
      advance(c);
    }
  }
}

} // namespace vga
