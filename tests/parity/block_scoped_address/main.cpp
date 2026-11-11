// Provisional: Go int and func types (here std::function) are not frozen mappings (see ../README.md).
#include <eo/fmt.h>

#include <functional>

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

// header declares y in an if statement's init, whose C++ scope ends with the statement.
auto header() -> int {
  ptr<int> p;
  std::function<int()> f;
  if (auto y = New<int>(3); *y > 0) {
    p = y;
    f = [=]() mutable -> int {
      (*y)++;
      return *y;
    };
  }
  *p += 10;
  return f();
}

func<> eo_main() {
  fmt::Println(value(), header());
  co_return;
}
