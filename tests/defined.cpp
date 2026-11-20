// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/fmt.h>

#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

using namespace eo;

void check(bool condition, const char* message) {
  if (!condition) {
    throw std::logic_error(message);
  }
}

// Runs f, which must panic, and returns the panic message.
template<typename F>
auto panic_message(F f) -> std::string {
  try {
    f();
  } catch (const Panic& p) {
    return p.message();
  }
  throw std::logic_error("expected a panic");
}

struct Prio : eo::defined<Prio, int8_t> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Prio";
};

struct Code : eo::defined<Code, uint8_t> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Code";
};

struct Temp : eo::defined<Temp, double> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Temp";
};

struct Celsius : eo::defined<Celsius, double> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Celsius";
};

struct Name : eo::defined<Name, std::string> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Name";
};

struct Data : eo::defined<Data, slice<uint8_t>> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Data";
};

struct Set : eo::defined<Set, map<std::string, bool>> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Set";
};

static_assert(eo::detail::defined_type<Prio> && eo::detail::defined_type<Name>);
static_assert(!std::is_aggregate_v<Prio> && !std::is_aggregate_v<Name>, "a defined type is not printed as a struct");
static_assert(!std::is_convertible_v<double, Celsius> && !std::is_convertible_v<Celsius, double>,
  "a numeric defined type converts only explicitly, as in Go");
static_assert(std::is_convertible_v<slice<uint8_t>, Data>, "a slice is assignable to a defined slice type");

void test_operators_keep_the_type_and_wrap() {
  static_assert(std::is_same_v<decltype(Prio{1} + Prio{2}), Prio>);
  check(Prio{127} + Prio{1} == Prio{-128}, "int8 addition should wrap");
  check(-Prio{-128} == Prio{-128}, "negating the most negative int8 should wrap");
  check(~Code{0u} == Code{255u} && (Code{200u} << 1) == Code{144u}, "uint8 operators should wrap");
  auto p = Prio{127};
  p++;
  check(p == Prio{-128}, "++ should wrap");
  p -= Prio{1};
  check(p == Prio{127}, "-= should wrap");
  check(Temp{1.5} < Temp{2.0} && !(Temp{2.0} < Temp{1.5}), "comparisons compare the values");
}

void test_helpers_take_defined_integers() {
  static_assert(std::is_same_v<decltype(eo::div(Prio{1}, Prio{1})), Prio>);
  check(panic_message([] { eo::div(Prio{1}, Prio{0}); }) == "runtime error: integer divide by zero",
    "division by zero should panic");
  check(panic_message([] { eo::rem(Code{1u}, Code{0u}); }) == "runtime error: integer divide by zero",
    "remainder by zero should panic");
  check(eo::div(Prio{-128}, Prio{-1}) == Prio{-128} && eo::rem(Prio{-128}, Prio{-1}) == Prio{0},
    "the most negative value divided by -1 should wrap");
  check(eo::shl(Code{1u}, 40) == Code{0u} && eo::shr(Prio{-8}, 100) == Prio{-1}, "wide shifts should saturate");
  auto c = Code{3u};
  eo::shl_assign(c, 2);
  eo::div_assign(c, Code{4u});
  check(c == Code{3u}, "the assignment helpers should take a defined type");
}

void test_conversions() {
  check(int64_t(Temp{-1.7}) == -1 && double(Prio{-3}) == -3.0, "a conversion to a number converts the value");
  check(Celsius(Temp{2.5}) == Celsius{2.5}, "a conversion between defined types keeps the value");
  check(Code(Prio{-1}) == Code{255u}, "a conversion between integer types keeps the low bits");
}

void test_float_keys() {
  auto m = map<Temp, int64_t>::of({{Temp{0.0}, 1}});
  eo::map_entry(m, Temp{-0.0}) = 2;
  auto nan = Temp{std::numeric_limits<double>::quiet_NaN()};
  eo::map_entry(m, nan) = 3;
  eo::map_entry(m, nan) = 4;
  check(eo::len(m) == 3 && !(nan == nan), "a NaN key should be a new entry each time");
  check(fmt::Sprint(m) == "map[NaN:3 NaN:4 -0:2]", "-0 should replace the key 0, as Go does");
  check(fmt::Sprint(map<Name, int64_t>::of({{Name{"b"s}, 1}, {Name{"a"s}, 2}})) == "map[a:2 b:1]",
    "keys of a defined string type should print in order");
}

void test_printing() {
  auto set = New<Set>(Set(map<std::string, bool>::of({{"a"s, true}})));
  auto got = fmt::Sprintf("%x|%X|%s|%d|%t|%v|%T|%v"s,
    Data(slice<uint8_t>::of({1, 171})),
    Name{"hi"s},
    Data(eo::bytes_from_string("ab"s)),
    Name{"x"s},
    Celsius{1.5},
    set,
    slice<Celsius>::of({}),
    Data{});
  check(got == "01ab|6869|ab|%!d(main.Name=x)|%!t(main.Celsius=1.5)|&map[a:true]|[]main.Celsius|[]",
    "a defined type should print as its underlying value, and a bad verb should name the defined type");
}

void test_interfaces_tell_defined_types_apart() {
  any a = Celsius{1.0};
  check(!eo::type_assert_ok<double>(a).second, "an interface holding Celsius should not hold a float64");
  check(a == any(Celsius{1.0}) && a != any(double{1.0}), "interface comparison should compare the dynamic types");
  check(panic_message([] { static_cast<void>(any(Data{}) == any(Data{})); }) ==
      "runtime error: comparing uncomparable type main.Data",
    "comparing interfaces holding a defined slice type should panic");
  check(any(Name{"a"s}) == any(Name{"a"s}) && any(Name{"a"s}) != any("a"s), "a defined string type is comparable");
}

int main() {
  test_operators_keep_the_type_and_wrap();
  test_helpers_take_defined_integers();
  test_conversions();
  test_float_keys();
  test_printing();
  test_interfaces_tell_defined_types_apart();
}
