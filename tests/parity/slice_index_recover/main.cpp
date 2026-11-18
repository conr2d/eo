// Provisional: func() as std::function, the C++ type of Go int, and untyped constants in composite
// literals and assignments are not frozen mappings (see ../README.md).
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
  auto s = slice<int64_t>::of({1, 2, 3});
  slice<int64_t> none;
  auto i = int64_t{5};
  auto j = int64_t{-1};
  auto k = int64_t{2};
  attempt("index"s, [=]() mutable { fmt::Println(eo::at(s, i)); });
  attempt("negative"s, [=]() mutable { fmt::Println(eo::at(s, j)); });
  attempt("nil"s, [=]() mutable { fmt::Println(eo::at(none, 0)); });
  attempt("assign"s, [=]() mutable { eo::at(s, i) = 4; });
  attempt("high"s, [=]() mutable { fmt::Println(eo::subslice(s, 0, i)); });
  attempt("low"s, [=]() mutable { fmt::Println(eo::subslice(s, i)); });
  attempt("inverted"s, [=]() mutable { fmt::Println(eo::subslice(s, k, k - 1)); });
  attempt("max"s, [=]() mutable { fmt::Println(eo::subslice(s, 0, 1, i)); });
  attempt("highAboveMax"s, [=]() mutable { fmt::Println(eo::subslice(s, 0, 3, k)); });
  attempt("lowAboveHigh"s, [=]() mutable { fmt::Println(eo::subslice(s, k, 1, 3)); });
  attempt("negativeLow3"s, [=]() mutable { fmt::Println(eo::subslice(s, j, 1, 3)); });
  attempt("negativeMax"s, [=]() mutable { fmt::Println(eo::subslice(s, 0, 1, j)); });
  attempt("makeLen"s, [=]() mutable { fmt::Println(eo::make_slice<int64_t>(j)); });
  attempt("makeCap"s, [=]() mutable { fmt::Println(eo::make_slice<int64_t>(k, k - 1)); });
  attempt("makeLenBelowCap"s, [=]() mutable { fmt::Println(eo::make_slice<int64_t>(j, k)); });
  attempt("ok"s, [=]() mutable {
    fmt::Println(eo::subslice(s, 1, 3),
      eo::at(s, 2),
      eo::subslice(s, 0, 0),
      eo::subslice(s, 3),
      eo::len(eo::subslice(s, 1, 1, 3)));
  });
  co_return;
}
