// Provisional: func() as std::function, and untyped constant arithmetic, are not frozen mappings (see ../README.md).
#include <eo/fmt.h>

#include <cstdint>
#include <functional>
#include <string>

using namespace eo;

void attempt(std::string name, std::function<void()> f) {
  eo_defer_scope;
  try {
    eo_defer([&] { fmt::Println(name, recover()); });
    f();
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
}

func<> eo_main() {
  auto s = "abc"s;
  auto i = int64_t{5};
  auto j = int64_t{-1};
  attempt("index"s, [=]() mutable { fmt::Println(at(s, i)); });
  attempt("negative"s, [=]() mutable { fmt::Println(at(s, j)); });
  attempt("high"s, [=]() mutable { fmt::Println(substr(s, 0, i)); });
  attempt("low"s, [=]() mutable { fmt::Println(substr(s, i)); });
  attempt("inverted"s, [=]() mutable { fmt::Println(substr(s, 2, 1 + j + 1)); });
  attempt("ok"s, [=]() mutable { fmt::Println(substr(s, 1, 3), at(s, 2)); });
  co_return;
}
