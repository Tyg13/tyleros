#ifndef LIBADT_INTRUSIVE_BITMAP_H
#define LIBADT_INTRUSIVE_BITMAP_H

#include "util.h"
#include <stdint.h>

namespace adt {

class bitmap {
  bitmap() = delete;

public:
  using limb_type = uint64_t;
  constexpr static auto BYTES_PER_LIMB = sizeof(limb_type);
  constexpr static auto BITS_PER_LIMB = BYTES_PER_LIMB * 8;

  static void make(bitmap &out, uint64_t num_bits) {
    out.num_limbs = kstd::div_ceil(num_bits, BITS_PER_LIMB);
    out.total_size = out.num_limbs * sizeof(limb_type);
    for (unsigned i = 0; i < out.num_limbs; ++i)
      out.limbs[i] = 0;
  }

  static uint64_t size_required(uint64_t num_bits) {
    return kstd::div_ceil(num_bits, BITS_PER_LIMB) * sizeof(limb_type);
  }

  bool test(unsigned idx) const {
    return (limbs[idx / BITS_PER_LIMB] & (1ULL << (idx % BITS_PER_LIMB))) != 0;
  }

  void set(unsigned idx) {
    limbs[idx / BITS_PER_LIMB] |= 1ULL << (idx % BITS_PER_LIMB);
  }

  void reset(unsigned idx) {
    limbs[idx / BITS_PER_LIMB] &= ~(1ULL << (idx % BITS_PER_LIMB));
  }

  int find_first(bool v) const {
    const limb_type empty = v ? 0 : ~0;
    for (unsigned i = 0; i < num_limbs; ++i) {
      if (limbs[i] == empty)
        continue;
      for (unsigned bit = 0; bit < BITS_PER_LIMB; ++bit)
        if ((bool)(limbs[i] & (1 << bit)) == v)
          return BITS_PER_LIMB * i + bit;
    }
    return -1;
  }

  int find_first_n(bool v, unsigned n) const {
    const limb_type empty = v ? 0 : ~0;
    unsigned num_contiguous = 0;
    int start_idx = -1;
    for (unsigned i = 0; i < size(); ++i) {
      if (num_contiguous >= n)
        return start_idx;
      if (limbs[i / BITS_PER_LIMB] == empty) {
        i += BITS_PER_LIMB - 1;
        num_contiguous = 0;
        start_idx = -1;
        continue;
      }
      if (limbs[i / BITS_PER_LIMB] == ~empty) {
        i += BITS_PER_LIMB - 1;
        num_contiguous += BITS_PER_LIMB;
        if (start_idx == -1)
          start_idx = i;
        continue;
      }
      unsigned j = i % BITS_PER_LIMB;
      for (; j < BITS_PER_LIMB && (i + j) < size(); ++j) {
        if (test(i + j) != v) {
          num_contiguous = 0;
          start_idx = -1;
          continue;
        }
        ++num_contiguous;
        if (start_idx == -1)
          start_idx = i + j;
        if (num_contiguous >= n)
          return start_idx;
      }
      i += BITS_PER_LIMB - (i % BITS_PER_LIMB) - 1;
    }
    return -1;
  }

  uint64_t size() const { return total_size; }

private:
  uint32_t num_limbs;
  uint32_t total_size;
  uint64_t limbs[];
};

} // namespace adt

#endif
