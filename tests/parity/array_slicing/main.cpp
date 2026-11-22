#include <eo/fmt.h>

#include <array>
#include <cstdint>
#include <string>

using namespace eo;

// The slice outlives the function, so the array is moved to heap.
auto digits() -> slice<int64_t> {
  auto a = New<std::array<int64_t, 4>>(std::array<int64_t, 4>{1, 2, 3, 4});
  auto s = eo::subslice(a, 1, 3);
  eo::at(*a, 1) = 20;
  return s;
}

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

func<> eo_main() {
  // A slice of an array shares its elements, and its capacity runs to the end of the array.
  auto a = New<std::array<int64_t, 5>>(std::array<int64_t, 5>{0, 1, 2, 3, 4});
  auto s = eo::subslice(a, 1, 3);
  eo::at(s, 0) = 10;
  eo::at(*a, 2) = 20;
  fmt::Println(*a, s, eo::len(s), eo::cap(s));
  auto t = eo::subslice(a, 0);
  auto u = eo::subslice(a, 2, 4, 4);
  eo::at(t, 4) = 40;
  fmt::Println(t, u, eo::len(u), eo::cap(u));
  s = eo::append(s, {30});
  fmt::Println(*a, s);
  u = eo::append(u, {50});
  eo::at(u, 0) = 0;
  fmt::Println(*a, u);
  auto n = eo::copy(eo::subslice(a, 3), slice<int64_t>::of({7, 8, 9}));
  fmt::Println(n, *a);

  auto d = digits();
  fmt::Println(d, eo::len(d), eo::cap(d));

  // A slice of a pointer to an array, and of an array field.
  auto p = New<std::array<std::string, 3>>(std::array<std::string, 3>{"a"s, "b"s, "c"s});
  auto ps = eo::subslice(p, 1);
  eo::at(ps, 0) = "B"s;
  fmt::Println(*p, ps);
  struct box {
    std::array<uint8_t, 4> buf{};

    auto operator==(const box&) const -> bool = default;
  };
  box bx{};
  eo::copy(eo::subslice(ref(bx.buf), 0), "hey!"s);
  fmt::Println(bx.buf, eo::string_from_bytes(eo::subslice(ref(bx.buf), 0, 3)));

  // Out-of-range bounds and a nil pointer panic.
  auto lo = int64_t{2};
  auto hi = int64_t{6};
  auto mx = int64_t{7};
  attempt("high"s, [=]() mutable { fmt::Println(eo::subslice(a, 0, hi)); });
  attempt("low"s, [=]() mutable { fmt::Println(eo::subslice(a, lo + 2, 3)); });
  attempt("max"s, [=]() mutable { fmt::Println(eo::subslice(a, 1, 2, mx)); });
  attempt("high3"s, [=]() mutable { fmt::Println(eo::subslice(a, 0, lo + 2, 3)); });
  ptr<std::array<int64_t, 3>> np;
  attempt("nil"s, [=]() mutable { fmt::Println(eo::subslice(np, 0)); });
  attempt("nil len"s, [=]() mutable { fmt::Println(eo::len(np), eo::cap(np)); });
  attempt("index"s, [=]() mutable { fmt::Println(eo::at(*np, lo)); });

  // Converting a slice to an array copies its first elements.
  auto src = slice<int64_t>::of({1, 2, 3, 4});
  auto arr = eo::array_from_slice<std::array<int64_t, 3>>(src);
  eo::at(src, 0) = 100;
  fmt::Println(arr, src);
  std::array<int64_t, 0> empty = eo::array_from_slice<std::array<int64_t, 0>>(src);
  fmt::Println(empty, eo::array_from_slice<std::array<int64_t, 0>>(slice<int64_t>{}));
  attempt("convert"s, [=]() mutable { fmt::Println(eo::array_from_slice<std::array<int64_t, 5>>(src)); });
  attempt("convert nil"s,
    [=]() mutable { fmt::Println(eo::array_from_slice<std::array<int64_t, 1>>(slice<int64_t>{})); });
  co_return;
}
