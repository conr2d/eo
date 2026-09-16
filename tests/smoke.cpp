#include <eo/core.h>

#include <cstdlib>

void register_defer(bool& deferred) {
  eo_defer_scope;
  eo_defer([&] { deferred = true; });
  eo_defer_run;
}

int main() {
  static_assert(__cplusplus >= 202100L, "Eo requires C++23 mode");

  bool deferred = false;
  register_defer(deferred);

  return deferred ? EXIT_SUCCESS : EXIT_FAILURE;
}
