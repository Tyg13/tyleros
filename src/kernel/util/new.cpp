#include "new.h"

#include "memory.h"

void * operator new  (size_t count) {
   return kmalloc(count);
}

void * operator new[](size_t count) {
   return kmalloc(count);
}

void operator delete  (void * p) noexcept {
   kfree(p);
}

void operator delete[](void * p) noexcept {
   kfree(p);
}

