#include "new.h"

#include "memory.h"

void *operator new(size_t count) { return memory::alloc(count); }

void *operator new[](size_t count) { return memory::alloc(count); }

void operator delete(void *p) noexcept { memory::free(p); }

void operator delete[](void *p) noexcept { memory::free(p); }
