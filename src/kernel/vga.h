#ifndef VGA_H
#define VGA_H

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

   constexpr auto SCREEN_WIDTH  = 80;
   constexpr auto SCREEN_HEIGHT = 25;

   struct cursor {
      int x;
      int y;
   };

   inline bool operator==(const cursor & lhs, const cursor & rhs) {
      return lhs.x == rhs.x && lhs.y == rhs.y;
   }
   inline bool operator!=(const cursor & lhs, const cursor & rhs) {
      return !(lhs == rhs);
   }

   extern const cursor null_cursor;

   class string {
      const char * text       = nullptr;
      color        background = color::black;
      color        foreground = color::white;
      cursor       position   = null_cursor;
   public:
      string() {}
      string(const char * _text) : text(_text) {}
      string& fg(color fg_color)      { foreground = fg_color;     return *this; }
      string& bg(color bg_color)      { background = bg_color;     return *this; }
      string& at(cursor new_position) { position   = new_position; return *this; }
      ~string();
   };

   extern bool initialized;
   void init();

   void clear_screen();

}

#endif
