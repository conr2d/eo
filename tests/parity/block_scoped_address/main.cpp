#include <eo/fmt.h>

#include <cstdint>

using namespace eo;

// value is a plain function, not a coroutine, so x is on the C++ stack.
auto value() -> int64_t {
  ptr<int64_t> p;
  {
    // Go keeps x on the stack, but its address outlives this C++ block, so x is boxed.
    auto x = New<int64_t>(5);
    p = x;
  }
  *p += 1;
  return *p;
}

// header declares y in an if statement's init, whose C++ scope ends with the statement.
auto header() -> int64_t {
  ptr<int64_t> p;
  fn<int64_t()> f{};
  if (auto y = New<int64_t>(3); *y > 0) {
    p = y;
    f = [=]() mutable -> int64_t {
      (*y)++;
      return *y;
    };
  }
  *p += 10;
  return f();
}

func<> eo_main() {
  auto _eo_call_0 = value();
  auto _eo_call_1 = header();
  fmt::Println(_eo_call_0, _eo_call_1);
  co_return;
}
