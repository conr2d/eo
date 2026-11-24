#include <eo/errors.h>
#include <eo/fmt.h>
#include <eo/strconv.h>

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

using namespace eo;

struct Celsius;
struct Count;
struct Name;
struct Hashes;
struct point;
struct item;
struct label;
struct code;
struct boom;

double zero;

struct Celsius : eo::defined<Celsius, double> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Celsius";
};

struct Count : eo::defined<Count, int64_t> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Count";
};

struct Name : eo::defined<Name, std::string> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Name";
};

struct Hashes : eo::defined<Hashes, slice<uint8_t>> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Hashes";
};

struct point {
  static constexpr std::string_view _eo_name = "main.point";

  int64_t X{}, Y{};

  auto operator==(const point&) const -> bool = default;
};

struct item {
  static constexpr std::string_view _eo_name = "main.item";

  std::string Name{};
  point Pos{};
  slice<std::string> Tags{};
  error Err{};
  any Value{};
};

struct label {
  static constexpr std::string_view _eo_name = "main.label";

  std::string text{};

  auto operator==(const label&) const -> bool = default;

  static auto String(label l) -> std::string;

  using _eo_methods = eo::methods<eo::method<&label::String, "String">>;
};

auto label::String(label l) -> std::string {
  return "<"s + l.text + ">"s;
}

struct code : eo::defined<code, int64_t> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.code";

  static auto GoString(code c) -> std::string;

  using _eo_methods = eo::methods<eo::method<&code::GoString, "GoString">>;
};

auto code::GoString(code c) -> std::string {
  return "code("s + strconv::Itoa(int64_t(c)) + ")"s;
}

struct boom {
  static constexpr std::string_view _eo_name = "main.boom";

  auto operator==(const boom&) const -> bool = default;

  static auto String(boom) -> std::string;

  using _eo_methods = eo::methods<eo::method<&boom::String, "String">>;
};

auto boom::String(boom) -> std::string {
  panic("boom"s);
}

func<> eo_main() {
  eo_defer_scope;
  try {
    auto nan = zero / zero;
    auto inf = 1 / zero;
    auto negZero = -zero;
    auto pi = double{0x1.921fb54442d18p+01};
    // Integers with widths, flags, and precisions.
    fmt::Printf("[%5d] [%-5d] [%05d] [%+d] [% d] [%+05d] [%-+5d]\n"s,
      int64_t{42},
      int64_t{42},
      int64_t{42},
      int64_t{42},
      int64_t{42},
      int64_t{-42},
      int64_t{7});
    fmt::Printf("[%x] [%X] [%#x] [%#X] [%#o] [%O] [%b] [%#b] [%o]\n"s,
      int64_t{255},
      int64_t{255},
      int64_t{255},
      int64_t{255},
      int64_t{8},
      int64_t{8},
      int64_t{5},
      int64_t{5},
      int64_t{-8});
    fmt::Printf("[%08.3d] [%.0d] [%5.0d] [%.3x] [%-#10x] [%#08x] [%x]\n"s,
      int64_t{7},
      int64_t{0},
      int64_t{0},
      int64_t{10},
      int64_t{255},
      int64_t{255},
      int64_t{-255});
    fmt::Printf("[%d] [%x] [%#o] [%03d] [%3d]\n"s,
      int64_t{-9223372036854775807 - 1},
      uint64_t{18446744073709551615u},
      int64_t{0},
      int64_t{-5},
      int64_t{12345});
    fmt::Printf("[%c] [%5c] [%-5c] [%q] [%+q] [%5q] [%c]\n"s,
      int32_t{65},
      int32_t{233},
      int32_t{122},
      int32_t{233},
      int32_t{233},
      int32_t{120},
      int64_t{1114112});
    fmt::Printf("[%U] [%#U] [%.6U] [%#U] [%10U] [%-10U]\n"s,
      int64_t{128512},
      int32_t{120},
      int64_t{65},
      int32_t{10},
      int32_t{97},
      int32_t{97});
    fmt::Printf("[%6v] [%-6v] [%06v] [%v]\n"s, Count{3}, Count{3}, uint8_t{9u}, Count{-1});

    // Strings and bytes.
    fmt::Printf("[%10s] [%-10s] [%.3s] [%.2s] [%10.2s] [%05s]\n"s,
      "hello"s,
      "hello"s,
      "hello"s,
      "héllo"s,
      "héllo"s,
      "ab"s);
    fmt::Printf("[%q] [%+q] [%#q] [%#q] [%#+q] [%.3q]\n"s,
      "héllo\n"s,
      "héllo"s,
      "back`tick"s,
      "plain"s,
      "é"s,
      "abcdef"s);
    fmt::Printf("[%x] [% x] [%#x] [% #x] [%X] [%.2x] [%8x] [%-8x] [%x]\n"s,
      "abc"s,
      "abc"s,
      "abc"s,
      "abc"s,
      "abc"s,
      "abc"s,
      "ab"s,
      "ab"s,
      ""s);
    fmt::Printf("[%s] [%x] [% X] [%q] [%5s] [%v] [%d] [%3v]\n"s,
      eo::bytes_from_string("hi"s),
      eo::bytes_from_string("hi"s),
      eo::bytes_from_string("hi"s),
      eo::bytes_from_string("hi"s),
      eo::bytes_from_string("hi"s),
      eo::bytes_from_string("hi"s),
      eo::bytes_from_string("hi"s),
      slice<uint8_t>::of({1, 2}));
    fmt::Printf("[%8v] [%-8v] [%8q] [%x]\n"s, Name{"go"s}, Name{"go"s}, Name{"go"s}, Name{"go"s});

    // Floats.
    fmt::Printf("[%.2f] [%8.3f] [%-8.2f] [%+.1f] [% .1f] [%08.2f] [%-08.2f] [%+08.2f] [%08.2f]\n"s,
      double{0x1.921f9f01b866ep+01},
      double{0x1.921f9f01b866ep+01},
      double{0x1.921f9f01b866ep+01},
      double{0x1.921f9f01b866ep+01},
      double{0x1.921f9f01b866ep+01},
      double{0x1.921f9f01b866ep+01},
      double{0x1.921f9f01b866ep+01},
      double{0x1.921f9f01b866ep+01},
      double{-0x1.921f9f01b866ep+01});
    fmt::Printf("[%e] [%.3e] [%E] [%g] [%.3g] [%G] [%.10g] [%g] [%g]\n"s,
      double{0x1.e240c9fbe76c9p+16},
      double{0x1.e240c9fbe76c9p+16},
      double{0x1.01f31f46ed246p-13},
      double{0x1.e240c9fbe76c9p+16},
      double{0x1.e240c9fbe76c9p+16},
      double{0x1.ad7f29abcaf48p-24},
      double{0x1.5555555555555p-02},
      double{1e+21},
      double{100000.0});
    fmt::Printf("[%#g] [%#.3g] [%#.0f] [%#e] [%#.0e] [%#g] [%#v] [%#g]\n"s,
      double{1.0},
      double{2.0},
      double{3.0},
      double{4.0},
      double{5.0},
      double{1.23456789e+08},
      double{1.5},
      double{0.0});
    fmt::Printf("[%5.2f] [%+f] [%05f] [% f] [%010f] [%-10f] [%f] [%+.1e]\n"s, nan, nan, nan, nan, inf, -inf, -inf, inf);
    fmt::Printf("[%.0f] [%.0f] [%.0f] [%.1f] [%.2f] [%.3f] [%.20f]\n"s,
      double{0.5},
      double{1.5},
      double{2.5},
      double{0.25},
      double{0x1.0147ae147ae14p+00},
      double{-0x1.0624dd2f1a9fcp-11},
      double{0x1.999999999999ap-04});
    fmt::Printf("[%v] [%v] [%v] [%v] [%v] [%v] [%.3v] [%10.4v]\n"s,
      double{1e+20},
      double{1e+21},
      double{0x1.4f8b588e368f1p-17},
      double{1.23456789e+08},
      double{0x1.9e3abe16fc70dp-17},
      negZero,
      pi,
      pi);
    fmt::Printf("[%b] [%x] [%X] [%.3x] [%#x] [%x] [%.0x] [%x]\n"s,
      double{1.0},
      double{1.0},
      double{-0.5},
      pi,
      double{1.0},
      double{0.0},
      double{1.5},
      double{0x1p-1074});
    fmt::Printf("[%v] [%g] [%.3f] [%e] [%b] [%x]\n"s,
      float{0x1.99999ap-04f},
      float{1e+10f},
      float{2.5f},
      float{0x1.555556p-02f},
      float{1.0f},
      float{0x1.99999ap-04f});
    fmt::Printf("[%6.2f] [%v] [%.1f] [%F] [%e]\n"s,
      Celsius{0x1.574bc6a7ef9dbp+04},
      Celsius{-3.0},
      Celsius{0x1.999999999999ap-05},
      double{1.5},
      double{0.0});

    // Booleans, nil, and types.
    fmt::Printf("[%6t] [%-6t] [%6v] [%5v] [%-5v] [%d] [%10T] [%-10T]\n"s,
      true,
      false,
      true,
      nullptr,
      nullptr,
      nullptr,
      int64_t{3},
      "s"s);

    // Widths and precisions from operands.
    fmt::Printf("[%*d] [%-*d] [%*d] [%.*f] [%*.*f] [%.*s]\n"s,
      int64_t{5},
      int64_t{42},
      int64_t{5},
      int64_t{42},
      int64_t{-5},
      int64_t{42},
      int64_t{2},
      pi,
      int64_t{8},
      int64_t{3},
      pi,
      int64_t{2},
      "hello"s);
    fmt::Printf("[%*d] [%.*d] [%*d] [%.*d]\n"s,
      "x"s,
      int64_t{1},
      "y"s,
      int64_t{2},
      int64_t{10000000},
      int64_t{3},
      int64_t{-1},
      int64_t{4});
    fmt::Printf("[%*d] [%*d]\n"s, uint8_t{3u}, int64_t{1}, Count{4}, int64_t{2});
    fmt::Printf("[%*d]\n"s, int64_t{3});

    // Operand indexes.
    fmt::Printf("[%[2]d %[1]d] [%[1]d %d] [%[3]*.[2]*[1]f] [%[5]d] [%[0]d] [%[x]d] [%[1]]\n"s,
      int64_t{1},
      int64_t{2},
      int64_t{9});
    fmt::Printf("[%[2]d]\n"s, int64_t{1}, int64_t{2}, int64_t{3});
    fmt::Printf("[%d %d]\n"s, int64_t{1}, int64_t{2}, int64_t{3}, "x"s, nullptr);
    fmt::Printf("[%-]\n"s);
    fmt::Printf("[%[1]5d]\n"s, int64_t{3});
    fmt::Printf("%[3]v %[1]v %v\n"s, "a"s, "b"s, "c"s);

    // Structs with field names, and Go syntax.
    auto p = New<point>(point{1, -2});
    auto it = item{.Name = "box"s, .Pos = point{3, 4}, .Tags = slice<std::string>::of({"a"s, "b"s})};
    fmt::Printf("%v | %+v | %#v\n"s, *p, *p, *p);
    fmt::Printf("%+v\n%#v\n"s, it, it);
    fmt::Printf("%+v | %#v | %+v\n"s, p, p, slice<point>::of({point{1, 2}, point{3, 4}}));
    fmt::Printf("[%6v] [%-4v] [%+5v]\n"s, *p, *p, *p);
    fmt::Printf("%#v %#v %#v %#v %#v %#v\n"s, int64_t{42}, uint64_t{42u}, "q\"s"s, true, double{1e+06}, int8_t{-3});
    fmt::Printf("%#v %#v %#v %#v\n"s,
      slice<int64_t>::of({1, 2}),
      slice<std::string>{},
      slice<int64_t>::of({}),
      std::array<int64_t, 3>{1, 2, 3});
    fmt::Printf("%#v %#v %#v\n"s,
      map<std::string, int64_t>::of({{"b"s, 2}, {"a"s, 1}}),
      map<std::string, int64_t>{},
      map<int64_t, bool>::of({}));
    fmt::Printf("%#v %#v %#v %#v\n"s,
      slice<uint8_t>::of({1, 2}),
      slice<uint8_t>{},
      Hashes{slice<uint8_t>::of({0xab})},
      Hashes{});
    fmt::Printf("%#v %#v %#v %#v\n"s, Count{5}, Name{"n"s}, Celsius{1.5}, std::array<uint8_t, 2>{1, 2});
    fmt::Printf("%#v %#v %v %+v\n"s, code{7}, slice<code>::of({code{1}, code{2}}), code{7}, label{"x"s});
    fmt::Printf("%#v | %#v\n"s, any{}, slice<error>::of({nullptr}));
    fmt::Printf("%#v\n"s, slice<any>::of({int64_t{1}, "a"s, nullptr, point{}}));

    // Methods with flags.
    auto l = label{"hi"s};
    auto err = errors::New("bad thing"s);
    fmt::Printf("[%10v] [%-10s] [%.3s] [%x] [%q] [%8v] [%-12v]\n"s, l, l, l, l, l, err, err);
    fmt::Printf("[%10v] [%-8d]\n"s, boom{}, label{"x"s});
    fmt::Printf("[%5z] [%-6z] [%z]\n"s, int64_t{3}, "ab"s, l);
    fmt::Printf("[%p] [%p] [%p] [%p] [%v]\n"s,
      ptr<int64_t>{},
      slice<int64_t>{},
      map<int64_t, int64_t>{},
      int64_t{5},
      ptr<int64_t>{});
    fmt::Printf("[%6.2f%%] [%5%] [%-5%]\n"s, double{99.5});
    auto wrapped = fmt::Errorf("op %[2]s: %[1]w"s, err, "read"s);
    fmt::Println(wrapped, errors::Is(wrapped, err));
    fmt::Println(fmt::Sprintf("%08.3f|%-8.3f|%+.2e"s, double{-1.5}, double{2.25}, double{0x1.81cd6c8b43958p+13}));

    // strconv's float formatting and ASCII quoting.
    for (auto [_eo_range_key, f] : eo::range(slice<uint8_t>::of({98, 101, 69, 102, 103, 71, 120, 88, 122}))) {
      auto _eo_call_0 = strconv::FormatFloat(1234.5678, f, -1, 64);
      auto _eo_call_1 = strconv::FormatFloat(1234.5678, f, 2, 64);
      auto _eo_call_2 = strconv::FormatFloat(-0.000123, f, 3, 32);
      fmt::Println(eo::string_from_rune(f), _eo_call_0, _eo_call_1, _eo_call_2);
    }
    auto _eo_call_3 = strconv::FormatFloat(1e23, 'g', -1, 64);
    auto _eo_call_4 = strconv::FormatFloat(5e-324, 'e', -1, 64);
    auto _eo_call_5 = strconv::FormatFloat(0.1, 'f', 30, 32);
    auto _eo_call_6 = strconv::FormatFloat(123, 'g', 2, 64);
    fmt::Println(_eo_call_3, _eo_call_4, _eo_call_5, _eo_call_6);
    fmt::Println(strconv::FormatFloat(1.7976931348623157e308, 'f', 0, 64));
    auto _eo_call_7 = strconv::AppendFloat(eo::bytes_from_string("x="s), 2.5, 'f', 1, 64);
    auto _eo_call_8 = strconv::FormatFloat(0, 'g', 3, 64);
    auto _eo_call_9 = strconv::FormatFloat(100, 'g', 1, 64);
    auto _eo_call_10 = strconv::FormatFloat(1e-5, 'g', 1, 64);
    fmt::Println(eo::string_from_bytes(_eo_call_7), _eo_call_8, _eo_call_9, _eo_call_10);
    auto _eo_call_11 = strconv::QuoteToASCII("héllo\x{01}☺"s);
    auto _eo_call_12 = strconv::QuoteRuneToASCII(9786);
    auto _eo_call_13 = strconv::QuoteRuneToASCII(97);
    auto _eo_call_14 = strconv::QuoteRune(9786);
    fmt::Println(_eo_call_11, _eo_call_12, _eo_call_13, _eo_call_14);
    auto _eo_call_15 = strconv::CanBackquote("plain text"s);
    auto _eo_call_16 = strconv::CanBackquote("tab\there"s);
    auto _eo_call_17 = strconv::CanBackquote("new\nline"s);
    auto _eo_call_18 = strconv::CanBackquote("`"s);
    auto _eo_call_19 = strconv::CanBackquote("\ufeff"s);
    auto _eo_call_20 = strconv::CanBackquote("\x{ff}"s);
    fmt::Println(_eo_call_15, _eo_call_16, _eo_call_17, _eo_call_18, _eo_call_19, _eo_call_20);
    eo_defer([&] { fmt::Println("recovered:"s, recover()); });
    strconv::FormatFloat(1, 'g', -1, 16);
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
  co_return;
}
