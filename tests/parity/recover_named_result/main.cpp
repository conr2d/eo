#include <eo/fmt.h>

using namespace eo;

auto divide(int a, int b) -> int {
  int q{};
  eo_defer_scope;
  try {
    eo_defer([&] {
      if (auto r = recover(); r != nullptr) {
        fmt::Println("recovered:", r);
        q = -1;
      }
    });
    if (b == 0) {
      panic("division by zero");
    }
    q = a / b;
    eo_defer_run;
    return q;
  } catch (...) {
    eo_defer_panic;
  }
  return q;
}

auto set() -> int {
  int n{};
  eo_defer_scope;
  try {
    eo_defer([&] { recover(); });
    eo_defer([&] { panic("late"); });
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
  fmt::Println("recover outside panic:", recover());
  fmt::Println("result kept after late panic:", set());
  co_return;
}
