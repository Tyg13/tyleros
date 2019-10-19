#include "stdlib.h"
#include "stdio.h"

char * itoa(int value, char * str, int base) {
   switch (base) {
      case 10:
         sprintf(str, "%d", value);
         return str;
      case 16:
         sprintf(str, "%x", value);
         return str;
      default:
         // unimplemented
         return NULL;
   }
}
