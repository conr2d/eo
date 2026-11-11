// Provisional: struct types, composite literals, and the C++ type of Go int are not frozen mappings (see ../README.md).
#include <eo/fmt.h>

using namespace eo;

struct T {
  int n{};
};

auto read(ptr<T> p) -> int {
  int n{};
  eo_defer_scope;
  try {
    eo_defer([&] {
      if (auto r = recover(); r != nullptr) {
        fmt::Println("recovered:"s, r);
        n = -1;
      }
    });
    n = p->n;
    eo_defer_run;
    return n;
  } catch (...) {
    eo_defer_panic;
  }
  return n;
}

func<> eo_main() {
  fmt::Println(read(New<T>(T{.n = 3})));
  fmt::Println(read(nullptr));
  co_return;
}
