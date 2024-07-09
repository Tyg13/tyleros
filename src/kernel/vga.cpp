#include "vga.h"

#include "mutex.h"
#include "paging.h"

#include <string.h>
#include <stdint.h>

namespace vga {

struct color_char {
  static color_char make(char c, color bg, color fg) {
    return color_char(((uint16_t)fg << 8 | (uint16_t)bg << 12 | (uint16_t)c));
  }

  uint16_t val = 0;
};

const cursor null_cursor = {.x = -1, .y = -1};

constexpr auto VGA_MEMORY_BASE_ADDRESS = 0xB8000;
auto *VGA_MEMORY = (uint16_t*)VGA_MEMORY_BASE_ADDRESS;

static void write_color_char_at(const char c, color fg, color bg, cursor pos);
kstd::managed_by_mutex<cursor> current_cursor(cursor{.x = 0, .y = 0});

void advance_y(cursor &cursor) {
  if (cursor.y + 1 == SCREEN_HEIGHT - 1)
    memmove(&VGA_MEMORY[0], &VGA_MEMORY[SCREEN_WIDTH],
            SCREEN_WIDTH * (SCREEN_HEIGHT - 1) * sizeof(uint16_t));
  else
    cursor.y += 1;
};
void advance(cursor &cursor) {
  if (++cursor.x >= SCREEN_WIDTH)
    cursor.x = 0;
  // If wrapping occurred
  if (cursor.x == 0) {
    advance_y(cursor);
  }
};
void cursor::newline() {
  x = 0;
  advance_y(*this);
};

void string::write_impl(cursor &current) const {
  if (position != null_cursor) {
    current = position;
  }
  auto *cursor = text;
  while (*cursor != '\0') {
    char c = *cursor;
    if (c == '\n') {
      current.x = 0;
      advance_y(current);
    } else if (c == '\b') {
      if (--current.x < 0) {
        current.x = 0;
      }
      write_color_char_at(' ', color::black, color::black, current);
    } else {
      write_color_char_at(c, foreground, background, current);
      advance(current);
    }
    ++cursor;
  }
}

void string::write() const {
  write_impl(*current_cursor.lock());
}

void string::print(const char *text) {
  string(text).write();
}

void string::puts(const char *text) {
  auto current = current_cursor.lock();
  string(text).write_impl(*current);
  current->newline();
}

void string::print_char(char c) {
  const char s[2] = {c, '\0'};
  print(s);
}

static void write_color_char_at(const char c, color fg, color bg, cursor pos) {
  auto vga_memory_base = reinterpret_cast<uint16_t *>(VGA_MEMORY_BASE_ADDRESS);
  auto *screen_pos = vga_memory_base + pos.x + pos.y * SCREEN_WIDTH;
  auto color = (uint16_t)((unsigned char)bg << 4 | (unsigned char)fg);
  *screen_pos = color << 8 | (uint16_t)c;
}

bool initialized = false;
void init() {
  // Identity map the entire VGA buffer.
  paging::kernel_page_tables.identity_map_range_size(
      VGA_MEMORY_BASE_ADDRESS, SCREEN_HEIGHT * SCREEN_WIDTH * sizeof(uint16_t));
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

void vga_print(const char *text) {
  vga::string::print(text);
}
