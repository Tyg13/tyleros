#ifndef ADT_INTRUSIVE_BITMAP_H
#define ADT_INTRUSIVE_BITMAP_H

#include "util.h"

#include <string.h>
#include <stdint.h>

namespace kstd {

class intrusive_bitmap {
public:
  using limb_type = uint8_t;
  constexpr static auto bytes_per_limb = sizeof(limb_type);
  constexpr static auto bits_per_limb = bytes_per_limb * 8;

  static uint64_t size_in_limbs(uint64_t size) {
    return div_ceil(size, bits_per_limb);
  }

  static uint64_t size_in_bytes(uint64_t size) {
    return size_in_limbs(size) * bytes_per_limb;
  }

  intrusive_bitmap(uint64_t size, bool v)
      : num_limbs(intrusive_bitmap::size_in_limbs(size)),
        limbs((limb_type *)(this + 1)) {
    memset(limbs, (int)v, num_limbs * bytes_per_limb);
  }

  intrusive_bitmap(uint64_t size) : intrusive_bitmap(size, false) {}

  bool test(unsigned idx) const {
    return (limbs[idx / bits_per_limb] & (1 << (idx & bits_per_limb))) != 0;
  }

  void set(unsigned idx) {
    limbs[idx / bits_per_limb] |= 1 << (idx & bits_per_limb);
  }
  void clear(unsigned idx) {
    limbs[idx / bits_per_limb] &= ~(1 << (idx & bits_per_limb));
  }

  uint64_t size() const { return num_limbs * bits_per_limb; }

private:
  uint64_t num_limbs = 0;
  uint8_t *limbs = nullptr;
};
} // namespace kstd

#endif
