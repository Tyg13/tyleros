#include "ctype.h"
#include "stdio.h"

#define CHECK_EOF(ch) if (ch == EOF) return 0;

int isalnum(int ch) {
  CHECK_EOF(ch);
  return isalpha(ch) || isdigit(ch);
}
int isalpha(int ch) {
  CHECK_EOF(ch);
  return islower(ch) || isupper(ch);
}
int islower(int ch) {
  CHECK_EOF(ch);
  return 'a' <= ch && ch <= 'z';
}
int isupper(int ch) {
  CHECK_EOF(ch);
  return 'A' <= ch && ch <= 'Z';
}
int isdigit(int ch) {
  CHECK_EOF(ch);
  return '0' <= ch && ch <= '9';
}
int isprint(int ch) {
  return ' ' <= ch && ch <= '~';
}

int tolower(int ch) {
  if (isupper(ch))
    return ch - ('A' - 'a');
  return ch;
}

int toupper(int ch) {
  if (islower(ch))
    return ch + ('A' - 'a');
  return ch;
}
