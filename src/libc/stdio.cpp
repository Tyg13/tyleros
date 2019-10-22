#include "stdio.h"
#include "string.h"

#include <stdarg.h>

int sprintf(char * str, const char * fmt, ...) {
   if (!fmt) return -1;
   int chars = 0;
   const auto put_char = [&](char c) {
      if(str) *str++ = c;
      ++chars;
   };

   const auto put_digit = [&](auto value, auto base) {
      int i = chars;
      if (value == 0) {
         put_char('0');
         return;
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
      strrev(str, digits_len);
   };

   va_list args;
   va_start(args, fmt);

   for (auto c = *fmt; c != '\0'; c = *++fmt) {
      if (c != '%') {
         put_char(c);
      }
      else {
         auto next = *++fmt;
         if (next == '%') {
            put_char(c);
            put_char(next);
         }
         else {
            switch(next) {
               case 's': {
                  for (auto arg = va_arg(args, const char *); *arg != '\0'; ++arg) {
                     put_char(*arg);
                  }
                  break;
               }
               case 'd': {
                  auto dec = va_arg(args, int);
                  put_digit(dec, 10);
                  break;
               }
               case 'x': {
                  auto hex = va_arg(args, int);
                  put_digit(hex, 16);
                  break;
               }
            }
         }
      }
   }

   put_char('\0');

   va_end(args);

   return chars;
}
