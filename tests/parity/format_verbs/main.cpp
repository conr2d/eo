#include <eo/errors.h>
#include <eo/fmt.h>

#include <cstdint>
#include <string>
#include <string_view>

using namespace eo;

struct Pair;
struct Name;

struct Pair {
  static constexpr std::string_view _eo_name = "main.Pair";

  int64_t A{};
  std::string S;

  auto operator==(const Pair&) const -> bool = default;
};

struct Name {
  static constexpr std::string_view _eo_name = "main.Name";

  int64_t N{};

  auto operator==(const Name&) const -> bool = default;

  static auto String(Name n) -> std::string;

  using _eo_methods = eo::methods<eo::method<&Name::String, "String">>;
};

auto Name::String(Name n) -> std::string {
  return fmt::Sprintf("name-%d"s, n.N);
}

func<> eo_main() {
  fmt::Printf("%d %s %v %x %X %q %t %T %%\n"s,
    int64_t{42},
    "s"s,
    Pair{1, "a"s},
    int64_t{255},
    "hi"s,
    "q\"\n\x{01}é"s,
    true,
    Pair{});
  fmt::Printf("%v %d %x %o %b %c %q %U\n"s,
    int64_t{-7},
    int8_t{-3},
    uint8_t{200},
    int64_t{8},
    int64_t{5},
    int32_t{233},
    int32_t{97},
    int32_t{233});
  fmt::Printf("%v %g %e %f %v\n"s, double{1.5}, double{1e21}, double{1234.5678}, double{0.1}, float{0.1});
  fmt::Printf("%d|%s|%x|%q|%v|%s\n"s,
    slice<int64_t>::of({1, 2}),
    slice<std::string>::of({"a"s, "b"s}),
    eo::bytes_from_string("hi"s),
    eo::bytes_from_string("hi"s),
    eo::bytes_from_string("hi"s),
    slice<uint8_t>{});
  fmt::Printf("%d|%s|%x|%v|%v\n"s,
    Pair{1, "a"s},
    Pair{1, "a"s},
    Pair{255, "a"s},
    New<Pair>(Pair{1, "a"s}),
    slice<ptr<Pair>>::of({nullptr}));
  fmt::Printf("%v|%s|%d|%q|%x\n"s, Name{1}, Name{2}, Name{3}, Name{4}, Name{5});
  fmt::Printf("%d|%s|%z|%d\n"s, "s"s, int64_t{3}, int64_t{1});
  fmt::Printf("%d"s, int64_t{1}, "x"s, double{2.5});
  fmt::Println();
  fmt::Printf("%v|%d|%s|%T|%w\n"s, nullptr, nullptr, error(nullptr), nullptr, errors::New("x"s));
  fmt::Printf("%x|%X|%x|%q\n"s, int64_t{-255}, "hi"s, ""s, "\x{ff}a\u00ad\t"s);
  fmt::Printf("%t|%v|%s\n"s, int64_t{1}, double{1.5}, double{2.5});
  fmt::Printf("trail%"s);
  fmt::Println();
  auto s = fmt::Sprintf("%v and %x"s, slice<any>::of({int64_t{1}, "a"s, nullptr}), slice<int64_t>::of({10, 11}));
  fmt::Println(s, eo::len(s));
  fmt::Println(fmt::Sprint("a"s, "b"s, int64_t{1}, int64_t{2}, "c"s),
    fmt::Sprint(int64_t{1}, int64_t{2}),
    fmt::Sprintln("x"s, int64_t{3}) == "x 3\n"s);
  co_return;
}
