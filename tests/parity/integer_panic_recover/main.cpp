#include <eo/fmt.h>

#include <cstdint>
#include <string>

using namespace eo;

void attempt(std::string name, fn<void()> f) {
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
  auto zero = int64_t{0};
  attempt("div"s, [=]() mutable { fmt::Println(eo::div(int64_t{1}, zero)); });
  attempt("rem"s, [=]() mutable { fmt::Println(eo::rem(int8_t{5}, int8_t(zero))); });
  attempt("div_assign"s, [=]() mutable {
    uint8_t b{};
    eo::div_assign(b, b);
    fmt::Println(b);
  });
  attempt("shift"s, [=]() mutable {
    auto n = int64_t{-1};
    fmt::Println(eo::shl(int64_t{1}, n));
  });
  attempt("shift_assign"s, [=]() mutable {
    auto n = int8_t{-3};
    auto x = uint64_t{8u};
    eo::shr_assign(x, n);
    fmt::Println(x);
  });
  attempt("ok"s, [=]() mutable { fmt::Println(eo::div(int64_t{7}, zero + 2), eo::shl(int64_t{1}, zero + 64)); });
  co_return;
}
