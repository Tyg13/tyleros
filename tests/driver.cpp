#include <stdio.h>
#include <stdlib.h>
#include "harness.h"

int main() {
  auto code = EXIT_SUCCESS;

  for (unsigned i = 0; i < Test::MAX_TESTS; ++i) {
    auto *test = Test::get(i);
    if (test == nullptr)
      break;
    if (!test->run()) {
      fprintf(stderr, "test %d \"%s\" failed\n", i, test->get_name());
      code = EXIT_FAILURE;
    }
  }

  return code;
}
