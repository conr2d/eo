#include <eo/core.h>

#include <cstdlib>

int main() {
  static_assert(__cplusplus >= 202100L, "Eo requires C++23 mode");

  bool deferred = false;
  {
    eo_defer([&] { deferred = true; });
  }

  return deferred ? EXIT_SUCCESS : EXIT_FAILURE;
}
