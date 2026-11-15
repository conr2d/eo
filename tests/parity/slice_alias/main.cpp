// Provisional: the C++ type of Go int, and untyped constants in composite literals, appends, and
// assignments, are not frozen mappings (see ../README.md).
#include <eo/fmt.h>

#include <cstdint>
#include <string>

using namespace eo;

void sum(std::string label, slice<int64_t> xs) {
  auto total = int64_t{0};
  for (auto i = int64_t{0}; i < eo::len(xs); i++) {
    total += eo::at(xs, i);
  }
  if (eo::len(xs) > 0) {
    eo::at(xs, 0) = -eo::at(xs, 0);
  }
  fmt::Println(label, total, eo::len(xs), xs == nullptr);
}

func<> eo_main() {
  auto a = slice<int64_t>::of({1, 2, 3});
  auto b = a;
  eo::at(b, 0) = 9;
  fmt::Println(a, b);

  auto t = eo::subslice(a, 1, 2);
  fmt::Println(eo::len(t), eo::cap(t));
  t = eo::append(t, {7});
  fmt::Println(a, t);
  auto u = eo::append(t, {8});
  eo::at(u, 0) = 100;
  fmt::Println(a, t, u);

  auto limited = eo::subslice(a, 0, 1, 1);
  limited = eo::append(limited, {-1});
  fmt::Println(a, limited, eo::cap(eo::subslice(a, 1, 2, 2)));

  slice<int64_t> none;
  auto empty = slice<int64_t>::of({});
  fmt::Println(none == nullptr,
    empty == nullptr,
    eo::len(none),
    eo::cap(none),
    eo::subslice(none, 0, 0) == nullptr,
    eo::subslice(empty, 0, 0) == nullptr);
  fmt::Println(eo::append_slice(none, none) == nullptr, eo::append_slice(empty, none) == nullptr, none, empty);

  auto m = eo::make_slice<int64_t>(2, 5);
  fmt::Println(m, eo::len(m), eo::cap(m), eo::subslice(m, 0, 5));
  auto grown = eo::append(m, {1, 2, 3});
  eo::at(grown, 0) = 4;
  fmt::Println(m, grown, eo::subslice(m, 0, 5));

  auto c = slice<int64_t>::of({1, 2, 3, 4, 5});
  auto n = eo::copy(eo::subslice(c, 1), c);
  fmt::Println(n, c);
  c = eo::append_slice(eo::subslice(c, 0, 1), eo::subslice(c, 2));
  fmt::Println(c, eo::len(c), eo::cap(c));
  eo::clear(eo::subslice(c, 1, 3));
  fmt::Println(c);

  auto words =
    slice<slice<std::string>>::of({slice<std::string>::of({"a"s, "b"s}), nullptr, slice<std::string>::of({})});
  eo::at(words, 1) = eo::append(eo::at(words, 1), {"c"s});
  fmt::Println(words, eo::len(eo::at(words, 2)), eo::at(words, 2) == nullptr, eo::at(eo::at(words, 1), 0));

  auto bs = eo::bytes_from_string("héllo"s);
  eo::at(bs, 0) = 'H';
  bs = eo::append_slice(bs, " world"s);
  fmt::Println(eo::subslice(bs, 0, 3), eo::string_from_bytes(bs));
  n = eo::copy(bs, "J"s);
  fmt::Println(n, eo::string_from_bytes(eo::subslice(bs, 0, 5)));
  auto rs = eo::runes_from_string("héllo\x{ff}"s);
  fmt::Println(rs, eo::len(rs), eo::string_from_runes(eo::subslice(rs, 1, 3)), eo::string_from_runes(rs));

  sum("none"s, nullptr);
  sum("two"s, slice<int64_t>::of({1, 2}));
  auto args = slice<int64_t>::of({3, 4, 5});
  sum("spread"s, args);
  fmt::Println(args);
  co_return;
}
