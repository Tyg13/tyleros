#include "stdio.h"
#include "string.h"

#include <stdarg.h>
#include <stdint.h>

int vsprintf(char * str, const char * fmt, va_list args) {
   if (!fmt) {
      return -1;
   }

   int chars = 0;
   const auto put_char = [&](char c) {
      if(str) *str++ = c;
      ++chars;
   };

   const auto put_digit = [&](auto value, auto base) {
      char * digits_start = str;
      int i = chars;
      if (value == 0) {
         return put_char('0');
      }

      bool is_negative = false;
      if (value < 0) {
         is_negative = true;
         value = -value;
      }

      while (value != 0) {
         int rem = value % base;
         char digit = (rem > 9) ?
                        // If remainder is greater than 10, then base must be > 10
                        // Use letters after A to represent digits > 9
                        (rem - 10) + 'A' :
                        // Otherwise, use normal digits
                        rem + '0';
         put_char(digit);
         value /= base;
      }

      if (is_negative) {
         put_char('-');
      }

      int digits_len = chars - i;
      strrev(digits_start, digits_len);
   };

   enum length {
      none,
      long_int,
      long_long_int,
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
         default:
            return none;
      }
   };

   for (auto c = *fmt; c != '\0'; c = *++fmt) {
      if (c != '%') {
         put_char(c);
      }
      else {
         ++fmt;
         const auto length = get_length();
         if (*(fmt + 1) == '%') {
            put_char(c);
            put_char('%');
         }
         else {
            int base = 10;
            switch(*fmt) {
               case 's': {
                  for (auto arg = va_arg(args, const char *); *arg != '\0'; ++arg) {
                     put_char(*arg);
                  }
                  break;
               }
               case 'i':
               case 'd': {
                  switch (length) {
                     case long_long_int: {
                        auto arg = va_arg(args, long long int);
                        put_digit(arg, base);
                        break;
                     }
                     case long_int: {
                        auto arg = va_arg(args, long int);
                        put_digit(arg, base);
                        break;
                     }
                     default: {
                        auto arg = va_arg(args, int);
                        put_digit(arg, base);
                        break;
                     }
                  }
                  break;
               }
               case 'x':
                  base = 16;
               case 'u': {
                  switch (length) {
                     case long_long_int: {
                        auto arg = va_arg(args, unsigned long long int);
                        put_digit(arg, base);
                        break;
                     }
                     case long_int: {
                        auto arg = va_arg(args, unsigned long int);
                        put_digit(arg, base);
                        break;
                     }
                     default: {
                        auto arg = va_arg(args, unsigned int);
                        put_digit(arg, base);
                        break;
                     }
                  }
                  break;
               }
               case 'p': {
                  auto addr = (uintptr_t) va_arg(args, void *);
                  put_digit(addr, 16);
                  break;
               }
            }
         }
      }
   }

   put_char('\0');

   return chars;
}

int sprintf(char * str, const char * fmt, ...) {
   va_list args;
   va_start(args, fmt);

   int ret = vsprintf(str, fmt, args);

   va_end(args);

   return ret;
}
