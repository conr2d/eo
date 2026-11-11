// Provisional: the C++ type of Go int is not a frozen mapping (see ../README.md).
#include <eo/fmt.h>

using namespace eo;

// value is a plain function, not a coroutine, so x is on the C++ stack.
auto value() -> int {
  ptr<int> p;
  {
    // Go keeps x on the stack, but its address outlives this C++ block, so x is boxed.
    auto x = New<int>(5);
    p = x;
  }
  *p += 1;
  return *p;
}

func<> eo_main() {
  fmt::Println(value());
  co_return;
}
