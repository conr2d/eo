#include <eo/fmt.h>
#include <eo/math/bits.h>
#include <eo/strings.h>

#include <cstdint>
#include <string>
#include <tuple>

using namespace eo;

void attempt(std::string name, fn<void()> f) {
  eo_defer_scope;
  try {
    eo_defer([&] { fmt::Println(name, "recovered:"s, recover()); });
    f();
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
}

auto build() -> std::string {
  strings::Builder b;
  auto _eo_call_0 = b.Len();
  auto _eo_call_1 = b.String();
  fmt::Println(_eo_call_0, _eo_call_1 == ""s);
  b.Grow(10);
  auto _eo_call_2 = b.Len();
  auto _eo_call_3 = b.Cap();
  fmt::Println(_eo_call_2, _eo_call_3 >= 10);
  auto [n, err] = b.WriteString("héllo"s);
  fmt::Println(n, err, b.Len());
  b.WriteByte(32);
  int64_t m{};
  std::tie(m, err) = b.WriteRune(0x263A);
  fmt::Println(m, err);
  b.WriteRune(-1);
  int64_t k{};
  std::tie(k, err) = b.Write(slice<uint8_t>::of({33, 63}));
  fmt::Println(k, err, b.Len());
  b.Grow(100);
  auto _eo_call_4 = b.Len();
  auto _eo_call_5 = b.Cap();
  fmt::Println(_eo_call_4, _eo_call_5 >= 115);
  auto s = b.String();
  b.Reset();
  auto _eo_call_6 = b.Len();
  auto _eo_call_7 = b.String();
  fmt::Println(_eo_call_6, _eo_call_7 == ""s);
  b.WriteString("again"s);
  return s + " "s + b.String();
}

func<> eo_main() {
  fmt::Printf("%q\n"s, build());

  // A Builder that was written to and then copied panics on the next write through the copy.
  strings::Builder a;
  a.WriteString("x"s);
  auto c = a;
  attempt("copy"s, [=, &c]() mutable { c.WriteString("y"s); });
  auto _eo_call_0 = c.String();
  auto _eo_call_1 = c.Len();
  fmt::Println(_eo_call_0, _eo_call_1);
  strings::Builder zero;
  auto z = zero;
  z.WriteString("fine"s);
  fmt::Println(z.String());

  attempt("grow"s, [=]() mutable {
    strings::Builder g;
    g.Grow(-1);
  });
  attempt("repeat"s, [=]() mutable { strings::Repeat("ab"s, -1); });
  attempt("repeat overflow"s, [=]() mutable { strings::Repeat("ab"s, int64_t{4611686018427387904}); });
  attempt("replacer"s, [=]() mutable { strings::NewReplacer(slice<std::string>::of({"a"s, "b"s, "c"s})); });
  auto _eo_call_2 = strings::Repeat("ab"s, 0);
  auto _eo_call_3 = strings::Repeat(""s, 5);
  auto _eo_call_4 = strings::Repeat("-"s, 3);
  fmt::Printf("%q %q %q\n"s, _eo_call_2, _eo_call_3, _eo_call_4);
  auto _eo_call_5 = strings::IndexFunc("ab,c"s, [=](int32_t r) mutable -> bool { return r == ','; });
  auto _eo_call_6 = strings::LastIndexFunc("a,b,c"s, [=](int32_t r) mutable -> bool { return r == ','; });
  fmt::Println(_eo_call_5, _eo_call_6);
  auto [l, ok] = strings::CutPrefix("prefix-body"s, "prefix-"s);
  fmt::Println(l, ok);
  std::string r;
  std::tie(r, ok) = strings::CutSuffix("body.go"s, ".txt"s);
  fmt::Println(r, ok, bits::UintSize);
  co_return;
}
