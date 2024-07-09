#include "util/type_traits.h"

namespace kstd {
template <typename Element, typename Transform>
void transform(Element array[], int num_of_elements,
               Transform transform_element) {
  using ResultType = decltype(transform_element(array[0]));
  for (auto i = 0; i < num_of_elements; ++i) {
    if constexpr (is_same_v<ResultType, bool>) {
      if (transform_element(array[i])) {
        return;
      }
    } else {
      transform_element(array[i]);
    }
  }
}

template <typename Element, typename Compare>
void insertion_sort(Element a[], int n, Compare less) {
  for (auto i = 1; i < n; ++i) {
    Element v = a[i];
    auto j = i - 1;
    for (; j >= 0 && !less(a[j], v); --j) {
      a[j + 1] = a[j];
    }
    a[j + 1] = v;
  }
}
} // namespace kstd
