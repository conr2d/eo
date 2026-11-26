#include <eo/encoding/json.h>
#include <eo/fmt.h>
#include <eo/math/big.h>
#include <eo/reflect.h>

#include <string>
#include <string_view>

using namespace eo;

struct Amount;
struct Record;

struct Amount : eo::defined<Amount, big::Int> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Amount";
};

struct Record {
  static constexpr std::string_view _eo_name = "main.Record";

  big::Int Value{};
  ptr<big::Int> Ptr{};
  std::string Name{};

  auto operator==(const Record&) const -> bool = default;
};

void equal(big::Int a, big::Int b) {
  eo_defer_scope;
  try {
    eo_defer([&] { fmt::Println("recovered:"s, recover()); });
    fmt::Println(reflect::Value::Equal(reflect::ValueOf(a), reflect::ValueOf(b)));
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
}

func<> eo_main() {
  auto t = reflect::TypeFor<big::Int>();
  auto _eo_call_0 = reflect::TypeFor<Amount>().Kind();
  auto _eo_call_1 = reflect::TypeFor<ptr<big::Int>>().Elem().Kind();
  fmt::Println(t.Kind(), t.Kind() == reflect::Struct, _eo_call_0, _eo_call_1);

  big::Int zero{};
  auto five = big::NewInt(5);
  auto _eo_call_2 = reflect::Value::IsZero(reflect::ValueOf(zero));
  auto _eo_call_3 = reflect::Value::IsZero(reflect::ValueOf(*five));
  auto _eo_call_4 = reflect::Value::IsZero(reflect::Value::Elem(reflect::ValueOf(five)));
  fmt::Println(_eo_call_2, _eo_call_3, _eo_call_4);
  auto _eo_call_5 = big::NewInt(7);
  auto r = Record{.Ptr = _eo_call_5, .Name = "r"s};
  auto _eo_call_6 = reflect::Value::IsZero(reflect::Value::Field(reflect::ValueOf(r), 0));
  auto _eo_call_7 = reflect::Value::Kind(reflect::Value::Field(reflect::ValueOf(r), 0));
  fmt::Println(_eo_call_6, _eo_call_7);

  auto [out, err] = json::Marshal(r);
  fmt::Println(eo::string_from_bytes(out), err);
  std::tie(out, err) = json::Marshal(map<std::string, big::Int>::of({{"a"s, *five}}));
  fmt::Println(eo::string_from_bytes(out), err);
  auto back = New<Record>();
  err = json::Unmarshal(eo::bytes_from_string(R"({"Value":"0x1f","Ptr":12,"Name":"b"})"s), back);
  fmt::Println(err, big::Int::String(ptr(back, &back->Value)), back->Ptr, back->Name);
  err = json::Unmarshal(eo::bytes_from_string(R"({"Ptr":null})"s), back);
  fmt::Println(err, back->Ptr);
  err = json::Unmarshal(eo::bytes_from_string(R"({"Value":{"x":1}})"s), back);
  fmt::Println(err);
  slice<uint8_t> text{};
  std::tie(text, err) = big::Int::MarshalText(five);
  fmt::Println(eo::string_from_bytes(text), err);
  ptr<big::Int> nilInt{};
  std::tie(text, err) = big::Int::MarshalText(nilInt);
  fmt::Println(eo::string_from_bytes(text), err);
  std::tie(text, err) = big::Int::MarshalJSON(nilInt);
  fmt::Println(eo::string_from_bytes(text), err);
  auto _eo_call_8 = big::NewInt(-12);
  std::tie(text, err) = big::Int::AppendText(_eo_call_8, eo::bytes_from_string("n="s));
  fmt::Println(eo::string_from_bytes(text), err);
  auto z = New<big::Int>();
  auto _eo_call_9 = big::Int::UnmarshalText(z, eo::bytes_from_string("0b101"s));
  auto _eo_call_10 = big::Int::UnmarshalText(z, eo::bytes_from_string("1_000"s));
  auto _eo_call_11 = big::Int::UnmarshalText(z, eo::bytes_from_string("12x"s));
  fmt::Println(_eo_call_9, z, _eo_call_10, z, _eo_call_11);
  auto _eo_call_12 = big::Int::UnmarshalJSON(z, eo::bytes_from_string("null"s));
  auto _eo_call_13 = big::Int::UnmarshalJSON(z, eo::bytes_from_string("-77"s));
  fmt::Println(_eo_call_12, _eo_call_13, z);
  auto _eo_call_14 = big::NewInt(1);
  auto _eo_call_15 = big::NewInt(1);
  equal(*_eo_call_14, *_eo_call_15);
  co_return;
}
