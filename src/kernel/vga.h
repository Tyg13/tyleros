#ifndef VGA_H
#define VGA_H

#include "mutex.h"
namespace vga {

enum class color : unsigned char {
  black = 0,
  blue,
  green,
  cyan,
  red,
  magenta,
  brown,
  light_gray,
  dark_gray,
  light_blue,
  light_green,
  light_cyan,
  light_red,
  light_magenta,
  yellow,
  white = 0xF,
};

constexpr auto SCREEN_WIDTH = 80;
constexpr auto SCREEN_HEIGHT = 25;

struct cursor {
  int x;
  int y;

  void newline();
  void reset() { x = 0; y = 0; }
};

inline bool operator==(const cursor &lhs, const cursor &rhs) {
  return lhs.x == rhs.x && lhs.y == rhs.y;
}
inline bool operator!=(const cursor &lhs, const cursor &rhs) {
  return !(lhs == rhs);
}

extern const cursor null_cursor;
extern kstd::managed_by_mutex<cursor> current_cursor;

class string {
  const char *text = nullptr;
  color background = color::black;
  color foreground = color::white;
  cursor position = null_cursor;

  void write_impl(cursor &c) const;

public:
  string() = default;
  string(const char *_text) : text(_text) {}
  string &fg(color fg_color) {
    foreground = fg_color;
    return *this;
  }
  string &bg(color bg_color) {
    background = bg_color;
    return *this;
  }
  string &at(cursor new_position) {
    position = new_position;
    return *this;
  }
  void write() const;
  // print to current position
  static void print(const char *text);
  // print to current position + newline
  static void puts(const char *text);
  static void print_char(char c);
};

extern bool initialized;
void init();

void clear_screen();

} // namespace vga

extern "C" void vga_print(const char *text);

#endif
