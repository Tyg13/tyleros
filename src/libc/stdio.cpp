#include "stdio.h"
#include "string.h"

#include <stdarg.h>
#include <stdint.h>

int vsnprintf(char * str, size_t buf_size, const char * fmt, va_list args) {
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
       if (!str) return;
       *str++ = c;
   };

   const auto put_integer = [&](int64_t value, unsigned base, bool capitalize, bool is_signed) {
       char *digits_start = str;
       const size_t num_chars_before = num_chars;
       if (value == 0) {
           put_char('0');
           return;
       }

       const bool is_negative = is_signed && value > 0;
       if (is_negative) {
           value = -value;
       }

       const char start_hex_digit = capitalize ? 'A' : 'a';

       while (value != 0) {
           const int64_t rem = value % base;
           const char digit = (rem > 9) ?
               // If remainder is greater than 10, then base must be > 10
               // Use letters after A to represent digits > 9
               (rem - 10) + start_hex_digit :
               // Otherwise, use decimal digits
               rem + '0';
           put_char(digit);
           value /= base;
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
   };

   const auto integer_arg_of_length = [](va_list args, length l) -> int64_t {
       switch (l) {
           case long_long_int:
               return va_arg(args, long long int);
           case long_int:
               return va_arg(args, long int);
           case short_int:
               return (short) va_arg(args, int);
           default:
               return va_arg(args, int);
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
            return short_int;
         default:
            return none;
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
       const auto length = get_length();
       switch(*fmt) {
           case 's': {
               for (auto arg = va_arg(args, const char *); *arg != '\0'; ++arg) {
                   put_char(*arg);
               }
               break;
           }
           case 'c':
               put_char((unsigned char) va_arg(args, int));
               break;
           case 'o':
               put_integer(integer_arg_of_length(args, length), 8, false, true);
               break;
           case 'i': case 'd':
               put_integer(integer_arg_of_length(args, length), 10, false, true);
               break;
           case 'u': 
               put_integer(integer_arg_of_length(args, length), 10, false, false);
               break;
           case 'X':
               put_integer(integer_arg_of_length(args, length), 16, true, false);
               break;
           case 'x':
               put_integer(integer_arg_of_length(args, length), 16, false, false);
               break;
           case 'p':
               put_integer((uintptr_t)va_arg(args, void*), 16, false, false);
               break;
           case 'n':
               switch (length) {
                   case short_int:
                       *va_arg(args, short*) = (short) num_chars;
                   case long_int:
                       *va_arg(args, long int*) = (long int) num_chars;
                   case long_long_int:
                       *va_arg(args, long long int*) = (long long int) num_chars;
                   default:
                       *va_arg(args, int*) = (int) num_chars;
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

int snprintf(char * str, size_t buf_size, const char * fmt, ...) {
   va_list args;
   va_start(args, fmt);

   const int ret = vsnprintf(str, buf_size, fmt, args);

   va_end(args);

   return ret;
}

int vsprintf(char * str, const char * fmt, va_list args) {
    return vsnprintf(str, SIZE_MAX, fmt, args);
}

int sprintf(char * str, const char * fmt, ...) {
   va_list args;
   va_start(args, fmt);

   const int ret = vsprintf(str, fmt, args);

   va_end(args);

   return ret;
}
