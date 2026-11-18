#include <eo/fmt.h>

#include <cstdint>

using namespace eo;

auto divide(int64_t a, int64_t b) -> int64_t {
  int64_t q{};
  eo_defer_scope;
  try {
    eo_defer([&] {
      if (auto r = recover(); r != nullptr) {
        fmt::Println("recovered:"s, r);
        q = -1;
      }
    });
    if (b == 0) {
      panic("division by zero"s);
    }
    q = eo::div(a, b);
    eo_defer_run;
    return q;
  } catch (...) {
    eo_defer_panic;
  }
  return q;
}

auto set() -> int64_t {
  int64_t n{};
  eo_defer_scope;
  try {
    eo_defer([&] { recover(); });
    eo_defer([&] { panic("late"s); });
    n = 5;
    eo_defer_run;
    return n;
  } catch (...) {
    eo_defer_panic;
  }
  return n;
}

func<> eo_main() {
  fmt::Println(divide(7, 2));
  fmt::Println(divide(1, 0));
  fmt::Println("result kept after late panic:"s, set());
  co_return;
}
