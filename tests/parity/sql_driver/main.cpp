#include <eo/database/sql/driver.h>
#include <eo/fmt.h>

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>

using namespace eo;

struct ID;
struct Count;
struct Empty;

// ID is stored as its bytes, as go-ethereum's common.Hash is.
struct ID : eo::defined<ID, std::array<uint8_t, 4>> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.ID";

  static auto Value(ID _eo_param_id) -> std::tuple<driver::Value, error>;

  using _eo_methods = eo::methods<eo::method<&ID::Value, "Value">>;
};

// Count is stored as an int64, and a negative one is an error.
struct Count : eo::defined<Count, int64_t> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Count";

  static auto Value(ptr<Count> c) -> std::tuple<driver::Value, error>;

  using _eo_methods = eo::methods<eo::method<&Count::Value, "Value">>;
};

// Empty is stored as a nil []byte, which is a non-nil Value.
struct Empty {
  static constexpr std::string_view _eo_name = "main.Empty";

  static auto Value(Empty) -> std::tuple<driver::Value, error>;

  using _eo_methods = eo::methods<eo::method<&Empty::Value, "Value">>;

  auto operator==(const Empty&) const -> bool = default;
};

auto ID::Value(ID _eo_param_id) -> std::tuple<driver::Value, error> {
  auto id = New<ID>(_eo_param_id);
  return {eo::subslice(id, 0), nullptr};
}

auto Count::Value(ptr<Count> c) -> std::tuple<driver::Value, error> {
  if (*c < Count{0}) {
    return {nullptr, fmt::Errorf("negative count %d"s, int64_t(*c))};
  }
  return {int64_t(*c), nullptr};
}

auto Empty::Value(Empty) -> std::tuple<driver::Value, error> {
  slice<uint8_t> b{};
  return {b, nullptr};
}

// kind names the type of one of the values that a driver must handle.
auto kind(driver::Value v) -> std::string {
  if (auto _eo_type_switch = v; _eo_type_switch == nullptr) {
    return "nil"s;
  } else if (eo::holds<int64_t>(_eo_type_switch)) {
    return "int64"s;
  } else if (eo::holds<double>(_eo_type_switch)) {
    return "float64"s;
  } else if (eo::holds<bool>(_eo_type_switch)) {
    return "bool"s;
  } else if (eo::holds<slice<uint8_t>>(_eo_type_switch)) {
    return "bytes"s;
  } else if (eo::holds<std::string>(_eo_type_switch)) {
    return "string"s;
  }
  return "other"s;
}

void store(driver::Valuer v) {
  auto [value, err] = v.Value();
  fmt::Println(kind(value), value, value == nullptr, err);
  if (auto [b, ok] = eo::type_assert_ok<slice<uint8_t>>(value); ok) {
    fmt::Println("bytes"s, eo::len(b), b);
  }
}

func<> eo_main() {
  driver::Value empty{};
  fmt::Println(empty == nullptr, empty);
  auto values =
    slice<driver::Value>::of({nullptr, int64_t{-3}, double{2.5}, true, eo::bytes_from_string("raw"s), "text"s});
  for (auto [i, v] : eo::range(values)) {
    fmt::Println(i, kind(v), v);
  }
  fmt::Printf("%T %T %T\n"s, eo::at(values, 4), eo::at(values, 5), eo::at(values, 2));
  fmt::Println(kind(ID{}));
  any a = eo::at(values, 5);
  driver::Value back = a;
  fmt::Println(back == eo::at(values, 5), back == driver::Value(a), a == any("text"s));

  store(ID(std::array<uint8_t, 4>{1, 2, 3, 4}));
  auto id = New<ID>(ID(std::array<uint8_t, 4>{5, 6, 7, 8}));
  store(id);
  store(Empty{});
  auto c = New<Count>(Count{7});
  store(c);
  *c = Count{-1};
  store(c);
  driver::Valuer v = ID{};
  auto isID = eo::type_assert_ok<ID>(v).second;
  fmt::Println(isID);
  any i = c;
  if (auto [valuer, ok] = eo::type_assert_ok<driver::Valuer>(i); ok) {
    store(valuer);
  }
  auto ok = eo::type_assert_ok<driver::Valuer>(any(*c)).second;
  fmt::Println(ok);
  co_return;
}
