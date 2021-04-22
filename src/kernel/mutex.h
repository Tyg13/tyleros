#ifndef MUTEX_H
#define MUTEX_H

#include "scheduler.h"

struct mutex {
   mutex() {}
   void acquire() {
      while (!__sync_bool_compare_and_swap(&locked, false, true)) {
          scheduler::yield();
      }
   }
   void release() {
      locked = false;
   }
private:
   bool locked = false;
};

struct mutex_guard {
    mutex_guard(mutex& mtx) : mtx(mtx) { mtx.acquire(); }
   ~mutex_guard() { mtx.release(); }
private:
   mutex& mtx;
};

#endif
