#include "vga.h"

#include "mutex.h"
#include "paging.h"

#include <algorithm>
#include <string.h>
#include <stdint.h>

namespace vga {

const cursor null_cursor = {.x = -1, .y = -1};

constexpr auto VGA_MEMORY_BASE_ADDRESS = 0xB8000;
auto *VGA_MEMORY = (uint16_t*)VGA_MEMORY_BASE_ADDRESS;

static void write_color_char_at(uint16_t *base, cursor pos, const char c, color fg,
                                color bg);
kstd::managed_by_mutex<screen> current_screen{screen{VGA_MEMORY}};

static uint16_t make_color_char(const char c, color fg, color bg) {
  const auto color = (uint16_t)((unsigned char)bg << 4 | (unsigned char)fg);
  return (color << 8) | (uint16_t)c;
}

static void write_color_char_at(uint16_t *base, cursor pos, const char c,
                                color fg, color bg) {
  base[pos.y * SCREEN_WIDTH + pos.x] = make_color_char(c, fg, bg);
}

void advance_one_y(uint16_t *buffer, cursor &c) {
  if (c.y + 1 == SCREEN_HEIGHT - 1)
    memmove(buffer, &VGA_MEMORY[SCREEN_WIDTH],
            SCREEN_WIDTH * (SCREEN_HEIGHT - 1) * sizeof(uint16_t));
  else
    c.y += 1;
}

void advance_one(uint16_t *buffer, cursor &cursor) {
  if (cursor.x < SCREEN_WIDTH)
    ++cursor.x;
  else {
    cursor.x = 0;
    advance_one_y(buffer, cursor);
  }
}

void screen::advance_cursor_to_newline() {
  position.x = 0;
  advance_one_y(buffer, position);
}

void screen::print_char(char c, color fg, color bg) {
  if (c == '\n') {
    advance_cursor_to_newline();
  } else if (c == '\b') {
    if (position.x > 0)
      --position.x;
    write_color_char_at(buffer, position, ' ', color::black, color::black);
  } else {
    write_color_char_at(buffer, position, c, fg, bg);
    advance_one(buffer, position);
  }
}

void screen::clear() {
  for (int y = 0; y < SCREEN_HEIGHT; ++y)
    for (int x = 0; x < SCREEN_WIDTH; ++x) {
      write_color_char_at(buffer, cursor{x, y}, ' ', color::black,
                          color::black);
    }
  set_cursor(cursor{});
}

void string::write_impl(screen &s) const {
  if (position != null_cursor) {
    s.set_cursor(position);
  }
  auto *c = text;
  while (*c != '\0')
    s.print_char(*c++, foreground, background);
}

void string::write() const { write_impl(*current_screen.lock()); }

void string::print(const char *text) {
  string(text).write();
}

void string::puts(const char *text) {
  auto screen = current_screen.lock();
  string(text).write_impl(*screen);
  screen->advance_cursor_to_newline();
}

void string::print_char(char c) {
  current_screen.lock()->print_char(c, color::white, color::black);
}

bool initialized = false;
void init() {
  // Identity map the entire VGA buffer.
  paging::kernel_page_tables.identity_map_page_range_into_kernel_space(
      VGA_MEMORY_BASE_ADDRESS, SCREEN_HEIGHT * SCREEN_WIDTH * sizeof(uint16_t));
  current_screen.lock()->clear();
  initialized = true;
}

} // namespace vga

extern "C" void vga_print(const char *text) {
  if (vga::initialized)
    vga::string::print(text);
}
