#include <eo/cmp.h>
#include <eo/errors.h>
#include <eo/fmt.h>
#include <eo/math.h>
#include <eo/slices.h>
#include <eo/strconv.h>
#include <eo/strings.h>
#include <eo/unicode.h>

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>

using namespace eo;

// A linear congruential generator, so that both programs draw the same values.
auto state = uint64_t{88172645463325252u};

auto next() -> uint64_t {
  state = state * uint64_t{6364136223846793005u} + uint64_t{1442695040888963407u};
  return state;
}

auto pick(int64_t n) -> int64_t {
  return int64_t(eo::rem(next() >> 33, uint64_t(n)));
}

struct Celsius : eo::defined<Celsius, double> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Celsius";
};

struct Name : eo::defined<Name, std::string> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Name";
};

slice<double> specials{};

void constants() {
  fmt::Println("E"s, math::E, "Pi"s, math::Pi, "Phi"s, math::Phi);
  fmt::Println("Sqrt2"s, math::Sqrt2, "SqrtE"s, math::SqrtE, "SqrtPi"s, math::SqrtPi, "SqrtPhi"s, math::SqrtPhi);
  fmt::Println("Ln2"s, math::Ln2, "Log2E"s, math::Log2E, "Ln10"s, math::Ln10, "Log10E"s, math::Log10E);
  fmt::Println("MaxFloat32"s, math::MaxFloat32, "SmallestNonzeroFloat32"s, math::SmallestNonzeroFloat32);
  fmt::Println("MaxFloat64"s, math::MaxFloat64, "SmallestNonzeroFloat64"s, math::SmallestNonzeroFloat64);
  fmt::Println(math::MaxInt, math::MinInt, math::MaxInt8, math::MinInt8, math::MaxInt16, math::MinInt16);
  fmt::Println(math::MaxInt32, math::MinInt32, math::MaxInt64, math::MinInt64);
  fmt::Println(uint64_t{18446744073709551615u},
    math::MaxUint8,
    math::MaxUint16,
    math::MaxUint32,
    uint64_t{18446744073709551615u});
  int64_t n = int64_t{1099511627776};
  fmt::Println(n > math::MaxUint32, int32_t(n >> 20) < int32_t{32767}, float{0x1.fffffep+127f});
}

void bitsAndSigns() {
  auto _eo_call_0 = math::Float64bits(math::NaN());
  auto _eo_call_1 = math::Float64bits(math::Inf(1));
  auto _eo_call_2 = math::Float64bits(math::Inf(-7));
  fmt::Printf("%#x %#x %#x\n"s, _eo_call_0, _eo_call_1, _eo_call_2);
  auto _eo_call_3 = math::Float32bits(1.5f);
  auto _eo_call_4 = math::Float32frombits(0x7f7fffff);
  fmt::Printf("%#x %v\n"s, _eo_call_3, _eo_call_4);
  auto _eo_call_5 = math::Float64frombits(math::Float64bits(-2.75));
  auto _eo_call_6 = math::Float64frombits(1);
  fmt::Println(_eo_call_5, _eo_call_6);
  for (auto [_eo_range_key, x] : eo::range(specials)) {
    auto _eo_call_7 = math::Abs(x);
    auto _eo_call_8 = math::Signbit(x);
    auto _eo_call_9 = math::Copysign(3, x);
    auto _eo_call_10 = math::Copysign(x, -1);
    auto _eo_call_11 = math::IsNaN(x);
    auto _eo_call_12 = math::IsInf(x, 0);
    auto _eo_call_13 = math::IsInf(x, 1);
    auto _eo_call_14 = math::IsInf(x, -1);
    fmt::Println(x,
      _eo_call_7,
      _eo_call_8,
      _eo_call_9,
      _eo_call_10,
      _eo_call_11,
      _eo_call_12,
      _eo_call_13,
      _eo_call_14);
  }
}

void rounding() {
  for (auto [_eo_range_key, x] : eo::range(specials)) {
    auto _eo_call_0 = math::Floor(x);
    auto _eo_call_1 = math::Ceil(x);
    auto _eo_call_2 = math::Trunc(x);
    auto _eo_call_3 = math::Round(x);
    auto _eo_call_4 = math::RoundToEven(x);
    fmt::Println(x, _eo_call_0, _eo_call_1, _eo_call_2, _eo_call_3, _eo_call_4);
  }
  for (auto i = int64_t{0}; i < 200; i++) {
    auto _eo_call_5 = next();
    auto _eo_call_6 = pick(60);
    auto x =
      double(int64_t(_eo_call_5 >> 12) - int64_t{2251799813685248}) / double(eo::shl(int64_t{1}, uint64_t(_eo_call_6)));
    if (pick(4) == 0) {
      x = double(pick(2000) - 1000) / 2;
    }
    auto _eo_call_7 = math::Floor(x);
    auto _eo_call_8 = math::Ceil(x);
    auto _eo_call_9 = math::Trunc(x);
    auto _eo_call_10 = math::Round(x);
    auto _eo_call_11 = math::RoundToEven(x);
    fmt::Printf("%x %x %x %x %x %x\n"s, x, _eo_call_7, _eo_call_8, _eo_call_9, _eo_call_10, _eo_call_11);
  }
}

void minMax() {
  auto negZero = math::Copysign(0, -1);
  auto nan = math::NaN();
  auto inf = math::Inf(1);
  auto pairs = slice<std::array<double, 2>>::of({std::array<double, 2>{1, 2},
    std::array<double, 2>{2, 1},
    std::array<double, 2>{0, negZero},
    std::array<double, 2>{negZero, 0},
    std::array<double, 2>{nan, 1},
    std::array<double, 2>{1, nan},
    std::array<double, 2>{inf, nan},
    std::array<double, 2>{nan, -inf},
    std::array<double, 2>{-1, -inf},
    std::array<double, 2>{3, 3}});
  for (auto [_eo_range_key, p] : eo::range(pairs)) {
    auto x = eo::at(p, 0);
    auto y = eo::at(p, 1);
    auto _eo_call_0 = math::Max(x, y);
    auto _eo_call_1 = math::Min(x, y);
    auto _eo_call_2 = math::Dim(x, y);
    auto _eo_call_3 = math::Mod(x, y);
    auto _eo_call_4 = math::Nextafter(x, y);
    fmt::Println(x, y, _eo_call_0, _eo_call_1, _eo_call_2, _eo_call_3, _eo_call_4);
  }
  auto _eo_call_5 = math::Mod(7, -3);
  auto _eo_call_6 = math::Mod(-7, 3);
  auto _eo_call_7 = math::Mod(math::Inf(1), 2);
  auto _eo_call_8 = math::Mod(2, math::Inf(-1));
  auto _eo_call_9 = math::Mod(1, 0);
  fmt::Println(_eo_call_5, _eo_call_6, _eo_call_7, _eo_call_8, _eo_call_9);
  auto _eo_call_10 = math::Nextafter(0, -1);
  auto _eo_call_11 = math::Nextafter(1, 2);
  auto _eo_call_12 = math::Nextafter(math::MaxFloat64, math::Inf(1));
  fmt::Println(_eo_call_10, _eo_call_11, _eo_call_12);
  auto _eo_call_13 = math::Float64bits(math::Mod(1, 0));
  auto _eo_call_14 = math::Float64bits(math::Mod(math::Inf(-1), 1));
  auto _eo_call_15 = math::Float64bits(math::Mod(nan, 1));
  auto _eo_call_16 = math::Float64bits(math::Max(nan, 1));
  fmt::Printf("%#x %#x %#x %#x\n"s, _eo_call_13, _eo_call_14, _eo_call_15, _eo_call_16);
}

void decompose() {
  for (auto [_eo_range_key, x] : eo::range(specials)) {
    auto [frac, exp] = math::Frexp(x);
    auto [i, f] = math::Modf(x);
    auto _eo_call_0 = math::Ldexp(frac, exp);
    auto _eo_call_1 = math::Sqrt(x);
    fmt::Println(x, frac, exp, _eo_call_0, i, f, _eo_call_1);
  }
  auto _eo_call_2 = math::Ldexp(1, 1024);
  auto _eo_call_3 = math::Ldexp(1, -1075);
  auto _eo_call_4 = math::Ldexp(1, -1074);
  auto _eo_call_5 = math::Ldexp(0.75, -1073);
  auto _eo_call_6 = math::Ldexp(1, int64_t{1099511627776});
  auto _eo_call_7 = math::Ldexp(1, int64_t{-1099511627776});
  fmt::Println(_eo_call_2, _eo_call_3, _eo_call_4, _eo_call_5, _eo_call_6, _eo_call_7);
  auto _eo_call_8 = math::Sqrt(2);
  auto _eo_call_9 = math::Sqrt(-1);
  auto _eo_call_10 = math::Sqrt(math::Copysign(0, -1));
  auto _eo_call_11 = math::Sqrt(math::Inf(1));
  fmt::Println(_eo_call_8, _eo_call_9, _eo_call_10, _eo_call_11);
}

void cmpPackage() {
  auto _eo_call_0 = cmp::Compare(int64_t{1}, 2);
  auto _eo_call_1 = cmp::Compare(int64_t{2}, 1);
  auto _eo_call_2 = cmp::Compare(int64_t{3}, 3);
  auto _eo_call_3 = cmp::Compare("a"s, "b"s);
  auto _eo_call_4 = cmp::Compare("b"s, "ab"s);
  fmt::Println(_eo_call_0, _eo_call_1, _eo_call_2, _eo_call_3, _eo_call_4);
  auto nan = math::NaN();
  auto _eo_call_5 = cmp::Compare(nan, 1.0);
  auto _eo_call_6 = cmp::Compare(1.0, nan);
  auto _eo_call_7 = cmp::Compare(nan, nan);
  auto _eo_call_8 = cmp::Compare(0.0, math::Copysign(0, -1));
  fmt::Println(_eo_call_5, _eo_call_6, _eo_call_7, _eo_call_8);
  auto _eo_call_9 = cmp::Less(nan, 1.0);
  auto _eo_call_10 = cmp::Less(1.0, nan);
  auto _eo_call_11 = cmp::Less(nan, nan);
  auto _eo_call_12 = cmp::Less(int64_t{1}, 2);
  auto _eo_call_13 = cmp::Less("b"s, "a"s);
  fmt::Println(_eo_call_9, _eo_call_10, _eo_call_11, _eo_call_12, _eo_call_13);
  auto _eo_call_14 = cmp::Compare(Celsius{1.5}, Celsius{-3.0});
  auto _eo_call_15 = cmp::Less(Name{"x"s}, Name{"y"s});
  auto _eo_call_16 = cmp::Compare(uint8_t{200}, 100);
  fmt::Println(_eo_call_14, _eo_call_15, _eo_call_16);
  auto _eo_call_17 = cmp::Or(slice<int64_t>::of({0, 0, 3, 4}));
  auto _eo_call_18 = cmp::Or(slice<std::string>::of({""s, "x"s}));
  auto _eo_call_19 = cmp::Or<std::string>(nullptr);
  auto _eo_call_20 = cmp::Or(slice<Name>::of({Name{""s}, Name{"n"s}}));
  fmt::Println(_eo_call_17, _eo_call_18, _eo_call_19, _eo_call_20 == Name{"n"s});
}

void slicesPackage() {
  auto a = slice<int64_t>::of({1, 2, 3});
  auto _eo_call_0 = slices::Compare(a, slice<int64_t>::of({1, 2, 3}));
  auto _eo_call_1 = slices::Compare(a, slice<int64_t>::of({1, 2}));
  auto _eo_call_2 = slices::Compare(a, slice<int64_t>::of({1, 2, 3, 0}));
  auto _eo_call_3 = slices::Compare(a, slice<int64_t>::of({1, 3}));
  fmt::Println(_eo_call_0, _eo_call_1, _eo_call_2, _eo_call_3);
  auto _eo_call_4 = slices::Compare(slice<int64_t>{}, slice<int64_t>::of({}));
  auto _eo_call_5 = slices::Compare(slice<std::string>::of({"b"s}), slice<std::string>::of({"a"s, "z"s}));
  fmt::Println(_eo_call_4, _eo_call_5);
  auto nan = math::NaN();
  auto _eo_call_6 = slices::Compare(slice<double>::of({nan}), slice<double>::of({nan}));
  auto _eo_call_7 = slices::Compare(slice<double>::of({nan}), slice<double>::of({1}));
  auto _eo_call_8 = slices::Equal(slice<double>::of({nan}), slice<double>::of({nan}));
  fmt::Println(_eo_call_6, _eo_call_7, _eo_call_8);
  auto _eo_call_9 = slices::Equal(a, slice<int64_t>::of({1, 2, 3}));
  auto _eo_call_10 = slices::Equal(slice<int64_t>{}, slice<int64_t>::of({}));
  auto _eo_call_11 = slices::Equal(a, eo::subslice(a, 0, 2));
  fmt::Println(_eo_call_9, _eo_call_10, _eo_call_11);
  auto fold = [=](std::string x, std::string y) mutable -> bool { return strings::EqualFold(x, y); };
  auto _eo_call_12 =
    slices::EqualFunc(slice<std::string>::of({"Go"s, "EO"s}), slice<std::string>::of({"go"s, "eo"s}), fold);
  auto _eo_call_13 = slices::EqualFunc(slice<std::string>::of({"a"s}), slice<std::string>::of({"b"s}), fold);
  fmt::Println(_eo_call_12, _eo_call_13);
  auto byLen = [=](std::string x, int64_t y) mutable -> int64_t { return cmp::Compare(eo::len(x), y); };
  auto _eo_call_14 = slices::CompareFunc(slice<std::string>::of({"ab"s, "c"s}), slice<int64_t>::of({2, 1}), byLen);
  auto _eo_call_15 = slices::CompareFunc(slice<std::string>::of({"ab"s}), slice<int64_t>::of({3}), byLen);
  auto _eo_call_16 = slices::CompareFunc(slice<std::string>::of({"ab"s}), slice<int64_t>::of({2, 0}), byLen);
  fmt::Println(_eo_call_14, _eo_call_15, _eo_call_16);
  for (auto i = int64_t{0}; i < 50; i++) {
    auto x = eo::make_slice<int64_t>(pick(4));
    auto y = eo::make_slice<int64_t>(pick(4));
    for (auto j : eo::range(eo::len(x))) {
      eo::at(x, j) = pick(3);
    }
    for (auto j : eo::range(eo::len(y))) {
      eo::at(y, j) = pick(3);
    }
    auto _eo_call_17 = slices::Compare(x, y);
    auto _eo_call_18 = slices::Equal(x, y);
    fmt::Print(_eo_call_17, _eo_call_18, " "s);
  }
  fmt::Println();
}

void show(std::string s, int64_t bitSize) {
  auto [f, err] = strconv::ParseFloat(s, bitSize);
  auto ne = New<ptr<strconv::NumError>>();
  if (errors::As(err, ne)) {
    auto _eo_call_0 = errors::Is(err, strconv::ErrRange);
    auto _eo_call_1 = errors::Is(err, strconv::ErrSyntax);
    fmt::Printf("%q/%d: %v %x %v [%s %q %v range=%v syntax=%v]\n"s,
      s,
      bitSize,
      f,
      f,
      err,
      (*ne)->Func,
      (*ne)->Num,
      (*ne)->Err,
      _eo_call_0,
      _eo_call_1);
    return;
  }
  fmt::Printf("%q/%d: %v %x %v\n"s, s, bitSize, f, f, err);
}

void parseFloat() {
  auto inputs = slice<std::string>::of({""s,
    "0"s,
    "-0"s,
    "+0"s,
    "1"s,
    "-1.5"s,
    "+.5"s,
    "5."s,
    "."s,
    "-"s,
    "+"s,
    "e5"s,
    "1e"s,
    "1e+"s,
    "1e-5"s,
    "1E+05"s,
    "0.1"s,
    "1.7976931348623157e308"s,
    "1.7976931348623159e308"s,
    "1.797693134862315807e308"s,
    "-1e309"s,
    "1e-400"s,
    "-1e-400"s,
    "4.9e-324"s,
    "2.4703282292062327e-324"s,
    "2.4703282292062328e-324"s,
    "1e-320"s,
    "0e999999"s,
    "1e99999999999999999999"s,
    "0.000000000000000000000000000001e30"s,
    "123456789012345678901234567890"s,
    "3.4028234663852886e38"s,
    "3.4028235677973366e38"s,
    "3.4028235677973367e38"s,
    "1e-46"s,
    "1e-45"s,
    "inf"s,
    "Inf"s,
    "+INF"s,
    "-inf"s,
    "infinity"s,
    "-Infinity"s,
    "infin"s,
    "infinityx"s,
    "nan"s,
    "NaN"s,
    "-nan"s,
    "+nan"s,
    "nanx"s,
    "in"s,
    "0x1p0"s,
    "0x1.8p1"s,
    "-0x1P-2"s,
    "0x.8p1"s,
    "0x1"s,
    "0x1p"s,
    "0xp1"s,
    "0x"s,
    "0X1FFFFFFFFFFFFFp-52"s,
    "0x1.fffffffffffff8p1023"s,
    "0x1p-1074"s,
    "0x1p-1075"s,
    "0x1.8p-1075"s,
    "0x1p1024"s,
    "0x1.ffffffp127"s,
    "0x1.fffffefp127"s,
    "0x1_0p0"s,
    "0x_1p0"s,
    "1_000.5"s,
    "1__0"s,
    "0x1p1_0"s,
    "0x1p_10"s,
    "1.5x"s,
    "1.5 "s,
    " 1.5"s,
    "1..5"s,
    "1.5."s,
    "0.0.0"s,
    "1e5e5"s,
    "0b101"s,
    "0o17"s,
    "1e+-5"s,
    "+-1"s,
    "\u0661"s});
  for (auto [_eo_range_key, s] : eo::range(inputs)) {
    show(s, 64);
    show(s, 32);
  }
  show("1.5"s, 16);
  show(strings::Repeat("9"s, 400), 64);
  show("0."s + strings::Repeat("0"s, 400) + "1"s, 64);
  show("1"s + strings::Repeat("0"s, 330) + "e-330"s, 64);
}

void parseRounds() {
  auto digits = "0123456789"s;
  for (auto i = int64_t{0}; i < 300; i++) {
    strings::Builder b;
    if (pick(3) == 0) {
      b.WriteByte(45);
    }
    auto n = 1 + pick(30);
    for (auto j = int64_t{0}; j < n; j++) {
      auto _eo_call_0 = pick(10);
      b.WriteByte(eo::at(digits, _eo_call_0));
      if (j == pick(n) && !strings::Contains(b.String(), "."s)) {
        b.WriteByte(46);
      }
    }
    b.WriteString("e"s);
    b.WriteString(strconv::Itoa(pick(700) - 360));
    auto s = b.String();
    auto [f64, err64] = strconv::ParseFloat(s, 64);
    auto [f32, err32] = strconv::ParseFloat(s, 32);
    auto _eo_call_1 = math::Float64bits(f64);
    auto _eo_call_2 = math::Float32bits(float(f32));
    fmt::Printf("%s %x %v %x %v\n"s, s, _eo_call_1, err64 != nullptr, _eo_call_2, err32 != nullptr);
  }
  for (auto i = int64_t{0}; i < 200; i++) {
    auto x = math::Float64frombits(next());
    if (math::IsNaN(x)) {
      continue;
    }
    auto formats = slice<uint8_t>::of({101, 102, 103, 120});
    auto _eo_call_3 = pick(4);
    auto format = eo::at(formats, _eo_call_3);
    auto prec = pick(25) - 1;
    if (format == 'f' && math::Abs(x) > 1e30) {
      format = 103;
    }
    auto s = strconv::FormatFloat(x, format, prec, 64);
    auto [y, err] = strconv::ParseFloat(s, 64);
    auto _eo_call_4 = math::Float64bits(x);
    auto _eo_call_5 = math::Float64bits(y);
    fmt::Printf("%x %s %x %v\n"s, _eo_call_4, s, _eo_call_5, err);
  }
  // The midpoints of adjacent float32s are float64s, so their exact decimals test ties to even,
  // and the decimals just above and below them test the rounding next to a tie.
  for (auto i = int64_t{0}; i < 150; i++) {
    auto lo = math::Float32frombits(uint32_t(next() >> 33) & 0x7f7fffff);
    auto hi = math::Float32frombits(math::Float32bits(lo) + 1);
    auto mid = (double(lo) + double(hi)) / 2;
    auto exact = strconv::FormatFloat(mid, 101, 800, 64);
    std::string mant{};
    std::string exp{};
    std::tie(mant, exp, std::ignore) = strings::Cut(exact, "e"s);
    mant = strings::TrimRight(mant, "0"s);
    auto tie = mant + "e"s + exp;
    auto above = mant + "1e"s + exp;
    auto f = std::get<0>(strconv::ParseFloat(tie, 32));
    auto g = std::get<0>(strconv::ParseFloat(above, 32));
    auto _eo_call_6 = math::Float32bits(lo);
    auto _eo_call_7 = math::Float32bits(float(f));
    auto _eo_call_8 = math::Float32bits(float(g));
    auto _eo_call_9 = math::Float64bits(mid);
    fmt::Printf("%x %x %x %x\n"s, _eo_call_6, _eo_call_7, _eo_call_8, _eo_call_9);
  }
}

void unicodePredicates() {
  struct predicate {
    std::string name{};
    fn<bool(int32_t)> is{};
  };
  auto predicates = slice<predicate>::of({predicate{"IsControl"s, unicode::IsControl},
    predicate{"IsLetter"s, unicode::IsLetter},
    predicate{"IsUpper"s, unicode::IsUpper},
    predicate{"IsLower"s, unicode::IsLower},
    predicate{"IsTitle"s, unicode::IsTitle},
    predicate{"IsMark"s, unicode::IsMark},
    predicate{"IsNumber"s, unicode::IsNumber},
    predicate{"IsDigit"s, unicode::IsDigit},
    predicate{"IsPunct"s, unicode::IsPunct},
    predicate{"IsSymbol"s, unicode::IsSymbol},
    predicate{"IsGraphic"s, unicode::IsGraphic},
    predicate{"IsPrint"s, unicode::IsPrint},
    predicate{"IsSpace"s, unicode::IsSpace}});
  for (auto [_eo_range_key, p] : eo::range(predicates)) {
    auto count = int64_t{0};
    auto hash = uint64_t{14695981039346656037u};
    for (auto r = int32_t{-2}; r <= int32_t{1114113}; r++) {
      if (p.is(r)) {
        count++;
        hash = (hash ^ uint64_t(r)) * uint64_t{1099511628211u};
      }
    }
    fmt::Printf("%s %d %x\n"s, p.name, count, hash);
  }
  for (auto [_eo_range_key, r] : eo::range(slice<int32_t>::of(
         {97, 90, 53, 95, 32, 160, 170, 178, 189, 453, 768, 1632, 8232, 12288, 128512, -1, int32_t{1114112}}))) {
    fmt::Printf("%U"s, r);
    for (auto [_eo_range_key, p] : eo::range(predicates)) {
      if (p.is(r)) {
        fmt::Print(" "s, p.name);
      }
    }
    fmt::Println();
  }
}

func<> eo_init() {
  auto _eo_call_0 = math::Copysign(0, -1);
  auto _eo_call_1 = math::Inf(1);
  auto _eo_call_2 = math::Inf(-1);
  auto _eo_call_3 = math::NaN();
  specials = slice<double>::of({0,
    _eo_call_0,
    1,
    -1,
    0.5,
    -0.5,
    1.5,
    2.5,
    -2.5,
    3.5,
    0.49999999999999994,
    4503599627370495.5,
    double{4.503599627370497e+15},
    1e300,
    -1e-300,
    5e-324,
    math::MaxFloat64,
    _eo_call_1,
    _eo_call_2,
    _eo_call_3});
  co_return;
}

func<> eo_main() {
  constants();
  bitsAndSigns();
  rounding();
  minMax();
  decompose();
  cmpPackage();
  slicesPackage();
  parseFloat();
  parseRounds();
  unicodePredicates();
  co_return;
}
