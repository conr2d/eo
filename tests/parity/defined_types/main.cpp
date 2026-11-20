#include <eo/errors.h>
#include <eo/fmt.h>

#include <cstdint>
#include <string>
#include <string_view>

using namespace eo;

struct Celsius;
struct Fahrenheit;
struct Weekday;
struct Prio;
struct Counter;
struct Flag;
struct Name;
struct Data;
struct Set;
struct Op;
struct Kelvin;
struct Code;
struct Reading;

// Celsius is a temperature, with a String method.
struct Celsius : eo::defined<Celsius, double> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Celsius";

  static auto ToF(Celsius c) -> Fahrenheit;
  static auto String(Celsius c) -> std::string;

  using _eo_methods = eo::methods<eo::method<&Celsius::ToF, "ToF">, eo::method<&Celsius::String, "String">>;
};

struct Fahrenheit : eo::defined<Fahrenheit, double> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Fahrenheit";
};

// Weekday is an enumeration.
struct Weekday : eo::defined<Weekday, int64_t> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Weekday";

  static auto String(Weekday d) -> std::string;
  static auto Next(Weekday d) -> Weekday;

  using _eo_methods = eo::methods<eo::method<&Weekday::String, "String">, eo::method<&Weekday::Next, "Next">>;
};

struct Prio : eo::defined<Prio, int8_t> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Prio";
};

struct Counter : eo::defined<Counter, uint32_t> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Counter";

  static void Inc(ptr<Counter> c);

  using _eo_methods = eo::methods<eo::method<&Counter::Inc, "Inc">>;
};

struct Flag : eo::defined<Flag, bool> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Flag";
};

struct Name : eo::defined<Name, std::string> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Name";

  static auto Greet(Name n) -> std::string;

  using _eo_methods = eo::methods<eo::method<&Name::Greet, "Greet">>;
};

struct Data : eo::defined<Data, slice<uint8_t>> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Data";

  static auto Len(Data d) -> int64_t;

  using _eo_methods = eo::methods<eo::method<&Data::Len, "Len">>;
};

struct Set : eo::defined<Set, map<std::string, bool>> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Set";

  static auto Has(Set s, std::string k) -> bool;

  using _eo_methods = eo::methods<eo::method<&Set::Has, "Has">>;
};

struct Op : eo::defined<Op, fn<int64_t(int64_t)>> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Op";

  static auto Twice(Op f, int64_t x) -> int64_t;

  using _eo_methods = eo::methods<eo::method<&Op::Twice, "Twice">>;
};

struct Kelvin : eo::defined<Kelvin, double> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Kelvin";
};

struct Code : eo::defined<Code, uint8_t> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Code";

  static auto Error(Code c) -> std::string;

  using _eo_methods = eo::methods<eo::method<&Code::Error, "Error">>;
};

struct Reading {
  static constexpr std::string_view _eo_name = "main.Reading";

  Celsius T{};
  Weekday Day{};

  auto operator==(const Reading&) const -> bool = default;
};

constexpr Weekday Sunday = Weekday{0};
constexpr Weekday Monday = Weekday{1};
constexpr Weekday Tuesday = Weekday{2};

const Name Default = Name{"anon"s};

auto Celsius::ToF(Celsius c) -> Fahrenheit {
  return Fahrenheit(c * Celsius{9.0} / Celsius{5.0} + Celsius{32.0});
}

auto Celsius::String(Celsius c) -> std::string {
  return fmt::Sprintf("%gC"s, double(c));
}

auto Weekday::String(Weekday d) -> std::string {
  if (auto _eo_switch = d; _eo_switch == Sunday) {
    return "Sun"s;
  } else if (_eo_switch == Monday) {
    return "Mon"s;
  } else if (_eo_switch == Tuesday) {
    return "Tue"s;
  }
  return fmt::Sprintf("Weekday(%d)"s, int64_t(d));
}

auto Weekday::Next(Weekday d) -> Weekday {
  return (d + Weekday{1}) % Weekday{7};
}

void Counter::Inc(ptr<Counter> c) {
  (*c)++;
}

auto Name::Greet(Name n) -> std::string {
  return "hi "s + std::string(n);
}

auto Data::Len(Data d) -> int64_t {
  return eo::len(d);
}

auto Set::Has(Set s, std::string k) -> bool {
  return eo::map_get(s, k);
}

auto Op::Twice(Op f, int64_t x) -> int64_t {
  return f(f(x));
}

auto Code::Error(Code c) -> std::string {
  return fmt::Sprintf("code %d"s, uint8_t(c));
}

func<> eo_main() {
  auto c = Celsius{37.5};
  fmt::Println(c, Celsius::ToF(c), double(c), c > Celsius{30.0}, c + Celsius{0.5});
  auto f = Celsius::ToF(c);
  fmt::Println(f, Celsius(f - Fahrenheit{32.0}) * Celsius{5.0} / Celsius{9.0}, int64_t(c));
  fmt::Printf("%v %g %T %T\n"s, c, c, c, f);

  auto d = Tuesday;
  auto next = Weekday::Next;
  fmt::Println(d, Monday, Weekday::Next(d), next(Weekday{6}), Weekday{9}, int64_t(d));
  fmt::Printf("%v %d %s %T\n"s, d, d, d, d);

  Prio p = Prio{127};
  p++;
  Prio minus = Prio{-1};
  fmt::Println(p, eo::div(p, minus), eo::rem(p, minus), -p, p >> 1, int8_t(p));

  Counter n{};
  Counter::Inc(ref(n));
  Counter::Inc(ref(n));
  auto pc = ref(n);
  Counter::Inc(pc);
  uint64_t k = 33;
  fmt::Println(n, n ^ Counter{255u}, n & ~Counter{1u}, n % Counter{2u}, eo::shl(n, k), Counter{16u});

  auto ok = Flag{true};
  fmt::Println(ok, !ok, ok == Flag{true});
  fmt::Printf("%T %s\n"s, ok, ok);

  auto nm = Name{"eo"s};
  nm += Name{"!"s};
  fmt::Println(nm, Name::Greet(nm), eo::len(nm), Name(eo::substr(nm, 1)), nm < Name{"z"s}, nm + Name{"?"s}, Default);
  fmt::Printf("%q %T %d\n"s, nm, nm, nm);

  auto data = New<Data>(Data(slice<uint8_t>::of({1, 2})));
  *data = Data(eo::append(*data, {10}));
  fmt::Println(*data, Data::Len(*data), Data::Len(Data(eo::subslice(*data, 1))));
  fmt::Printf("%x %T %v\n"s, *data, *data, data);
  for (auto [i, b] : eo::range(*data)) {
    fmt::Println(i, b);
  }
  Data none{};
  fmt::Println(none == nullptr, eo::len(none), none);

  auto set = Set(map<std::string, bool>::of({{"a"s, true}}));
  eo::map_entry(set, "b"s) = true;
  fmt::Println(set, Set::Has(set, "b"s), Set::Has(set, "z"s), eo::len(set));

  Op op = [=](int64_t x) mutable -> int64_t { return x * 3; };
  fmt::Println(op(2), Op::Twice(op, 2), Op(nullptr) == nullptr);

  error err = Code{4u};
  auto [code, isCode] = eo::type_assert_ok<Code>(err);
  fmt::Println(err, code, isCode, errors::Is(err, Code{4u}));

  auto temps = map<Weekday, Celsius>::of({{Monday, Celsius{20.0}}, {Sunday, Celsius{18.5}}});
  fmt::Println(temps, eo::map_get(temps, Monday));

  auto r = Reading{.T = Celsius{1.0}, .Day = Monday};
  fmt::Println(r, r == Reading{Celsius{1.0}, Monday});

  fmt::Stringer s = d;
  fmt::Println(s, eo::type_assert<Weekday>(s) == Tuesday);
  if (auto _eo_type_switch = any(c); eo::holds<Weekday>(_eo_type_switch)) {
    auto v = eo::type_assert<Weekday>(_eo_type_switch);
    fmt::Println("weekday"s, v);
  } else if (eo::holds<Celsius>(_eo_type_switch)) {
    auto v = eo::type_assert<Celsius>(_eo_type_switch);
    fmt::Println("celsius"s, v);
  }

  auto days = slice<Weekday>::of({Sunday, Weekday{2}});
  fmt::Println(days, eo::len(days));

  auto size = Weekday{3};
  auto names = eo::make_slice<std::string>(int64_t(size));
  eo::at(names, int64_t(Monday)) = "mon"s;
  auto both = Flag(ok && !ok);
  auto steps = Weekday{20};
  eo::div_assign(steps, d);
  auto raw = eo::bytes_from_string("raw"s);
  auto kv = Kelvin(c) + Kelvin{273.15};
  fmt::Println(names, eo::len(names), both, steps, Name(eo::string_from_bytes(raw)), nm < Default, kv);
  fmt::Printf("%T %v\n"s, kv, Celsius(kv));

  auto letter = Weekday{79};
  auto keys = int64_t{0};
  for (auto [k, _eo_range_value] : eo::range(set)) {
    keys += eo::len(k);
  }
  for (auto [i, ch] : eo::range(Name{"é!"s})) {
    fmt::Println(i, ch);
  }
  fmt::Println(eo::string_from_rune(int32_t(letter)), eo::string_from_rune(int64_t(letter)), keys);
  co_return;
}
