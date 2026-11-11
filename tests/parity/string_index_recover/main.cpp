// Provisional: func() as std::function, the C++ types of Go int and uint, and untyped constant arithmetic are not
// frozen mappings (see ../README.md).
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
  auto u = uint64_t{9223372036854775808u};
  attempt("index"s, [=]() mutable { fmt::Println(eo::at(s, i)); });
  attempt("negative"s, [=]() mutable { fmt::Println(eo::at(s, j)); });
  attempt("high"s, [=]() mutable { fmt::Println(eo::substr(s, 0, i)); });
  attempt("low"s, [=]() mutable { fmt::Println(eo::substr(s, i)); });
  attempt("inverted"s, [=]() mutable { fmt::Println(eo::substr(s, 2, 1 + j + 1)); });
  attempt("unsigned"s, [=]() mutable { fmt::Println(eo::at(s, u)); });
  attempt("negativeLow"s, [=]() mutable { fmt::Println(eo::substr(s, j)); });
  attempt("negativeHigh"s, [=]() mutable { fmt::Println(eo::substr(s, 0, j)); });
  attempt("highFirst"s, [=]() mutable { fmt::Println(eo::substr(s, i, i + 1)); });
  attempt("ok"s, [=]() mutable { fmt::Println(eo::substr(s, 1, 3), eo::at(s, 2)); });
  co_return;
}
