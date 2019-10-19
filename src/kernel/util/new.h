#ifndef UTIL_NEW_H
#define UTIL_NEW_H

#include <stddef.h>

void * operator new  (size_t count);
void * operator new[](size_t count);

void operator delete  (void *) noexcept;
void operator delete[](void *) noexcept;

#endif
