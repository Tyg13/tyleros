#ifndef LIBC_PLATFORM_SPECIFIC_H
#define LIBC_PLATFORM_SPECIFIC_H

#include <stdint.h>

// clang-format off
#if TESTING_LIBC
#if !defined(__cplusplus)
#error "libc test environment should be C++"
#endif
#define LIBC_NAMESPACE_BEGIN extern "C++" { namespace kstd {
#define LIBC_NAMESPACE_END } }
#else
#if defined(__cplusplus)
#define LIBC_NAMESPACE_BEGIN extern "C" {
#define LIBC_NAMESPACE_END }
#else
#define LIBC_NAMESPACE_BEGIN
#define LIBC_NAMESPACE_END
#endif // __cplusplus
#endif // TESTING_LIBC

#if TESTING_LIBC
#define LIBC_NAMESPACE_PREFIX kstd::
#else
#define LIBC_NAMESPACE_PREFIX
#endif

#if defined(__cplusplus)
#if defined(__GNUC__) || defined(__clang__)
#define LIBC_RESTRICT __restrict
#else
#define LIBC_RESTRICT
#endif
#else
#define LIBC_RESTRICT restrict
#endif

#if defined(__cplusplus)
#if defined(__GNUC__) || defined(__clang__)
#define LIBC_NORETURN __attribute__((noreturn))
#else
#define LIBC_NORETURN
#endif
#else
#define LIBC_NORETURN _Noreturn
#endif

#if defined(__GNUC__) || defined(__clang)
#define LIBC_UNREACHABLE() __builtin_unreachable()
#else
#define LIBC_UNREACHABLE()
#endif

enum _sycall_codes {
  _SYSCALL_ABORT = 0ULL,
  _SYSCALL_EXIT  = 1ULL,
  _SYSCALL_ALLOC = 2ULL,
  _SYSCALL_FREE  = 3ULL,
  _SYSCALL_PRINT = 4ULL,
};
// clang-format on

static inline uint64_t _syscall0(uint64_t code) {
  uint64_t ret;
  asm volatile("int $0x80"
               : "=a"(ret)
               : "D"(code)
               : "rsi", "rdx", "rcx", "r8", "r9", "r10", "r11");
  return ret;
}

static inline uint64_t _syscall1(uint64_t code, uint64_t arg0) {
  uint64_t ret;
  asm volatile("int $0x80"
               : "=a"(ret)
               : "D"(code), "S"(arg0)
               : "rdx", "rcx", "r8", "r9", "r10", "r11");
  return ret;
}

static inline uint64_t _syscall2(uint64_t code, uint64_t arg0, uint64_t arg1) {
  uint64_t ret;
  asm volatile("int $0x80"
               : "=a"(ret)
               : "D"(code), "S"(arg0), "d"(arg1)
               : "rcx", "r8", "r9", "r10", "r11");
  return ret;
}

static inline uint64_t _syscall3(uint64_t code, uint64_t arg0, uint64_t arg1,
                                 uint64_t arg2) {
  uint64_t ret;
  asm volatile("int $0x80"
               : "=a"(ret)
               : "D"(code), "S"(arg0), "d"(arg1), "c"(arg2)
               : "r8", "r9", "r10", "r11");
  return ret;
}

#endif
