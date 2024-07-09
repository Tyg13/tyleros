#ifndef LIBC_ASSERT_H
#define LIBC_ASSERT_H

#ifdef NDEBUG
#define assert(...)
#else
#define assert(x)                                                              \
  do {                                                                         \
    if (!(x))                                                                  \
      (*_on_assert)(__FILE__, __LINE__, #x);                                   \
  } while (0)
#endif

typedef void (*_assert_handler)(const char *file, unsigned line,
                                const char *condition);
extern _assert_handler _on_assert;

#endif
