#include <eo/errors.h>
#include <eo/fmt.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>

using namespace eo;

struct T;
struct Loud;
struct Bad;

struct T {
  static constexpr std::string_view _eo_name = "main.T";

  int64_t A{};

  auto operator==(const T&) const -> bool = default;
};

struct Loud {
  static constexpr std::string_view _eo_name = "main.Loud";

  int64_t N{};

  auto operator==(const Loud&) const -> bool = default;

  static auto String(Loud l) -> std::string;

  using _eo_methods = eo::methods<eo::method<&Loud::String, "String">>;
};

struct Bad {
  static constexpr std::string_view _eo_name = "main.Bad";

  auto operator==(const Bad&) const -> bool = default;

  static auto String(Bad) -> std::string;

  using _eo_methods = eo::methods<eo::method<&Bad::String, "String">>;
};

auto Loud::String(Loud l) -> std::string {
  return fmt::Sprintf("loud-%d"s, l.N);
}

auto Bad::String(Bad) -> std::string {
  panic(Loud{.N = 7});
}

void classify(std::string name, fn<void()> f) {
  eo_defer_scope;
  try {
    eo_defer([&] {
      auto r = recover();
      auto [err, isErr] = eo::type_assert_ok<error>(r);
      auto isRuntime = eo::type_assert_ok<runtime::Error_>(r).second;
      fmt::Printf("%s: %T %v %t %t\n"s, name, r, r, isErr, isRuntime);
      if (isErr) {
        auto re = New<runtime::Error_>();
        auto _eo_call_0 = err.Error();
        auto _eo_call_1 = errors::As(err, re);
        fmt::Println("  message:"s, _eo_call_0, _eo_call_1);
      }
    });
    f();
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
}

func<> eo_main() {
  ptr<T> p;
  auto s = slice<int64_t>::of({});
  any i = "x"s;
  auto z = int64_t{0};
  classify("nil"s, [=]() mutable { fmt::Println(p->A); });
  classify("index"s, [=]() mutable { fmt::Println(eo::at(s, 3)); });
  classify("slice"s, [=]() mutable { fmt::Println(eo::subslice(s, 1, 5)); });
  classify("assert"s, [=]() mutable { fmt::Println(eo::type_assert<int64_t>(i)); });
  classify("divide"s, [=]() mutable { fmt::Println(eo::div(int64_t{1}, z)); });
  classify("nil panic"s, [=]() mutable { panic(nullptr); });
  classify("string"s, [=]() mutable { panic("str"s); });
  classify("error"s, [=]() mutable { panic(errors::New("boom"s)); });
  classify("wrapped"s, [=]() mutable { panic(fmt::Errorf("ctx: %w"s, errors::New("inner"s))); });
  classify("struct"s, [=]() mutable { panic(T{.A = 1}); });
  classify("stringer"s, [=]() mutable { panic(Loud{.N = 2}); });
  classify("repanic"s, [=]() mutable {
    eo_defer_scope;
    try {
      eo_defer([&] { panic(recover()); });
      panic(Loud{.N = 3});
      eo_defer_run;
    } catch (...) {
      eo_defer_panic;
    }
  });
  fmt::Println(Bad{});
  fmt::Printf("%s|%d\n"s, Bad{}, Bad{});
  co_return;
}
