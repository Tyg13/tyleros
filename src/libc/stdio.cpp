#include "./stdio.h"
#include "./string.h"
#include "./stdlib.h"
#include "./platform_specific.h"

#include <limits>
#include <stdarg.h>
#include <stdint.h>

LIBC_NAMESPACE_BEGIN
struct FILE { int _; };

int printf(const char *msg, ...) {
  va_list args;
  va_start(args, msg);

  int ret = vprintf(msg, args);

  va_end(args);
  return ret;
}

int vprintf(const char *msg, va_list args) {
  return vfprintf(stdout, msg, args);
}

int fprintf(FILE *f, const char * msg, ...) {
  va_list args;
  va_start(args, msg);

  int ret = vfprintf(f, msg, args);

  va_end(args);
  return ret;
}

#ifdef LIBC_IN_KERNEL
static char printf_buffer[0x200];
extern "C" void vga_print(const char *text);
extern "C" void debug_print(const char *text);
int vfprintf(FILE *f, const char *msg, va_list args) {
  vsnprintf(printf_buffer, 0x200, msg, args);
  if (f == stdout) {
    vga_print(printf_buffer);
    debug_print(printf_buffer);
  } else if (f == stderr) {
    debug_print(printf_buffer);
  } else {
    abort();
  }
  return 0;
}
#else
int vfprintf(FILE *f, const char *msg, va_list args) {
  return -1;
}
#endif

int vsnprintf(char *str, size_t buf_size, const char *fmt, va_list args) {
  if (!fmt) {
    return -1;
  }

  // Output a char into `str` and advance, unless the string is null,
  // or we've reached our buffer's size.
  auto buffer_full = false;
  size_t num_chars = 0;
  const auto put_char = [&](char c) {
    if (++num_chars >= buf_size) {
      buffer_full = true;
      return;
    }
    if (!str)
      return;
    *str++ = c;
  };

  enum flags { pad0 = 1 };
  const auto put_integer = [&](auto value, unsigned base, bool capitalize,
                               bool is_signed, unsigned max_digits, flags f) {
    char *digits_start = str;
    const size_t num_chars_before = num_chars;
    if (value == 0) {
      put_char('0');
      return;
    }

    const bool is_negative = is_signed && (value < 0);
    if (is_negative) {
      value = -value;
    }

    const char start_hex_digit = capitalize ? 'A' : 'a';

    unsigned num_digits = 0;
    while (value != 0 || ((f & pad0) != 0 && num_digits < max_digits)) {
      const int rem = value % base;
      const char digit =
          (rem > 9) ?
                    // If remainder is greater than 10, then base must be > 10
                    // Use letters after A to represent digits > 9
              (rem - 10) + start_hex_digit
                    :
                    // Otherwise, use decimal digits
              rem + '0';
      put_char(digit);
      value /= base;
      ++num_digits;
    }

    if (is_negative) {
      put_char('-');
    }

    if (!buffer_full && str) {
      const auto digits_len = num_chars - num_chars_before;
      strrev(digits_start, digits_len);
    }
  };

  enum length {
    none,
    long_int,
    long_long_int,
    short_int,
    char_int,
    size_int,
  };

  const auto signed_integer_arg_of_length = [](va_list args, length l) -> int64_t {
    switch (l) {
    case long_long_int:
      return va_arg(args, long long int);
    case long_int:
      return va_arg(args, long int);
    case short_int:
      return (short)va_arg(args, int);
    case char_int:
      return (char)va_arg(args, int);
    case size_int:
      return va_arg(args, size_t);
    default:
      return va_arg(args, int);
    }
  };

  const auto unsigned_integer_arg_of_length = [](va_list args, length l) -> uint64_t {
    switch (l) {
    case long_long_int:
      return va_arg(args, unsigned long long int);
    case long_int:
      return va_arg(args, unsigned long int);
    case short_int:
      return (unsigned short)va_arg(args, unsigned int);
    case char_int:
      return (unsigned char)va_arg(args, unsigned int);
    case size_int:
      return va_arg(args, size_t);
    default:
      return va_arg(args, unsigned int);
    }
  };

  const auto length_to_digits = [](length l, unsigned base) -> unsigned {
    switch (l) {
    default:
      return base == 10   ? std::numeric_limits<unsigned int>::digits10
             : base == 16 ? sizeof(unsigned int) * 2
             : base == 8  ? sizeof(unsigned int) * 4
                          : std::numeric_limits<unsigned>::max();
    case long_int:
      return base == 10   ? std::numeric_limits<long int>::digits10
             : base == 16 ? sizeof(long int) * 2
             : base == 8  ? sizeof(long int) * 4
                          : std::numeric_limits<unsigned>::max();
    case long_long_int:
      return base == 10   ? std::numeric_limits<long long int>::digits10
             : base == 16 ? sizeof(long long int) * 2
             : base == 8  ? sizeof(long long int) * 4
                          : std::numeric_limits<unsigned>::max();
    case short_int:
      return base == 10   ? std::numeric_limits<short int>::digits10
             : base == 16 ? sizeof(short int) * 2
             : base == 8  ? sizeof(short int) * 4
                          : std::numeric_limits<unsigned>::max();
    case char_int:
      return base == 10   ? std::numeric_limits<char>::digits10
             : base == 16 ? sizeof(char) * 2
             : base == 8  ? sizeof(char) * 4
                          : std::numeric_limits<unsigned>::max();
    case size_int:
      return base == 10   ? std::numeric_limits<size_t>::digits10
             : base == 16 ? sizeof(size_t) * 2
             : base == 8  ? sizeof(size_t) * 4
                          : std::numeric_limits<unsigned>::max();
    }
  };

  const auto get_length = [&]() {
    switch (*fmt) {
    case 'l':
      ++fmt;
      if (*fmt == 'l') {
        ++fmt;
        return long_long_int;
      }
      return long_int;
    case 'h':
      ++fmt;
      if (*fmt == 'h') {
        ++fmt;
        return char_int;
      }
      return short_int;
    case 'z':
      ++fmt;
      return size_int;
    default:
      return none;
    }
  };

  const auto get_flags = [&]() {
    switch (*fmt) {
      case '0':
        ++fmt;
        return flags::pad0;
      default: return flags{};
    }
  };

  for (auto c = *fmt; c != '\0'; c = *++fmt) {
    if (c != '%') {
      // Regular non-format specifier char
      put_char(c);
      continue;
    }
    ++fmt;
    if (*fmt == '%') {
      // Double %%, which is an escaped '%'
      put_char(c);
      continue;
    }

    // Format specifier
    const auto flags = get_flags();
    const auto length = get_length();
    switch (*fmt) {
    case 's': {
      for (auto arg = va_arg(args, const char *); *arg != '\0'; ++arg) {
        put_char(*arg);
      }
      break;
    }
    case 'c':
      put_char((unsigned char)va_arg(args, int));
      break;
    case 'o':
      put_integer(signed_integer_arg_of_length(args, length), 8, false, true,
                  length_to_digits(length, 8), flags);
      break;
    case 'i':
    case 'd':
      put_integer(signed_integer_arg_of_length(args, length), 10, false, true,
                  length_to_digits(length, 10), flags);
      break;
    case 'u':
      put_integer(unsigned_integer_arg_of_length(args, length), 10, false,
                  false, length_to_digits(length, 10), flags);
      break;
    case 'X':
      put_integer(unsigned_integer_arg_of_length(args, length), 16, true, false,
                  length_to_digits(length, 16), flags);
      break;
    case 'x':
      put_integer(unsigned_integer_arg_of_length(args, length), 16, false,
                  false, length_to_digits(length, 16), flags);
      break;
    case 'p':
      put_integer((uintptr_t)va_arg(args, void *), 16, false, false, 16,
                  flags::pad0);
      break;
    case 'n':
      switch (length) {
      case char_int:
        *va_arg(args, signed char *) = (signed char)num_chars;
      case short_int:
        *va_arg(args, short *) = (short)num_chars;
      case long_int:
        *va_arg(args, long int *) = (long int)num_chars;
      case long_long_int:
        *va_arg(args, long long int *) = (long long int)num_chars;
      case size_int:
        *va_arg(args, size_t *) = (size_t)num_chars;
      default:
        *va_arg(args, int *) = (int)num_chars;
      }
      break;
    }
  }

  if (!buffer_full && buf_size > 0 && str != nullptr) {
    // Null terminate buffer
    *str = '\0';
  }

  return num_chars;
}

int snprintf(char *str, size_t buf_size, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  const int ret = vsnprintf(str, buf_size, fmt, args);

  va_end(args);

  return ret;
}

int vsprintf(char *str, const char *fmt, va_list args) {
  return vsnprintf(str, SIZE_MAX, fmt, args);
}

int sprintf(char *str, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  const int ret = vsprintf(str, fmt, args);

  va_end(args);

  return ret;
}

#ifdef LIBC_IN_KERNEL
extern "C" void vga_puts(const char *text);
extern "C" void debug_puts(const char *text);
int fputs(const char *str, FILE *stream) {
  if (stream == stdout) {
    vga_print(str);
    debug_print(str);
  } else if (stream == stderr) {
    debug_print(str);
  }
  return 0;
}
#else
int fputs(const char *str, FILE *stream) {
  return EOF;
}
#endif

int puts(const char *str) {
  fputs(str, stdout);
  fputs("\n", stdout);
  return 0;
}

static FILE files[0x100] = {};
FILE *stdout = &files[0];
FILE *stderr = &files[1];

FILE *fopen(const char *path, const char *mode) {
  return nullptr;
}
LIBC_NAMESPACE_END
