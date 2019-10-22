#ifndef UTIL_IO_H
#define UTIL_IO_H

#include "type_traits.h"

#include <stdint.h>

#ifdef __clang__
#define __inline__
#else
#define __inline__ inline
#endif

namespace io {

   namespace {
      inline void _wait() {
         // Port 0x80 is a debug port used by the BIOS during POST.
         // Sould be safe for us to use to wait for out/in calls to properly finish.
         asm volatile __inline__ ("outb %%al, $0x80" :: "a"(0) );
      }
   }

   template <typename T> struct is_register           { constexpr static auto value = false; };
   template <>           struct is_register<uint8_t>  { constexpr static auto value = true; };
   template <>           struct is_register<uint16_t> { constexpr static auto value = true; };
   template <>           struct is_register<uint32_t> { constexpr static auto value = true; };

   template<typename Size>
   kstd::enable_if_t<is_register<Size>::value, void> out(kstd::dont_deduce<Size> port, kstd::dont_deduce<Size> val);
   template<>
   inline void out<uint8_t>(uint8_t port, uint8_t val) {
      asm volatile __inline__ ("outb %0, %1" : : "a"(val), "Ng"(port));
   }
   template<>
   inline void out<uint16_t>(uint16_t port, uint16_t val) {
      asm volatile __inline__ ("outw %0, %1" : : "a"(val), "Ng"(port)); }
   template<>
   inline void out<uint32_t>(uint32_t port, uint32_t val) {
      asm volatile __inline__ ("outl %0, %1" : : "a"(val), "Ng"(port));
   }

   struct wait {};

   template<typename Size>
   void out(kstd::dont_deduce<Size> port, kstd::dont_deduce<Size> val, io::wait) {
      out<Size>(port, val);
      io::_wait();
   }

   template<typename Size>
   kstd::enable_if_t<is_register<Size>::value, Size> in(kstd::dont_deduce<Size> port);
   template<>
   inline uint8_t in<uint8_t>(uint8_t port) {
      uint8_t val;
      asm volatile __inline__ ("inb %1, %0" : "=a"(val) : "Ng"(port));
      return val;
   }
   template<>
   inline uint16_t in<uint16_t>(uint16_t port) {
      uint16_t val;
      asm volatile __inline__ ("inw %1, %0" : "=a"(val) : "Ng"(port));
      return val;
   }
   template<>
   inline uint32_t in<uint32_t>(uint32_t port) {
      uint32_t val;
      asm volatile __inline__ ("inl %1, %0" : "=a"(val) : "Ng"(port));
      return val;
   }
}

#endif
