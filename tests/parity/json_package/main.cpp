#include <eo/bytes.h>
#include <eo/encoding/json.h>
#include <eo/errors.h>
#include <eo/fmt.h>
#include <eo/math.h>
#include <eo/strconv.h>
#include <eo/strings.h>
#include <eo/sync.h>

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>

using namespace eo;

struct Inner;
struct Embedded;
struct Hex;
struct Wrapped;
struct wrappedJSON;
struct Config;
struct PtrMarshal;
struct Holder;
struct Bad;
struct Node;
struct Small;
struct Tree;

struct Inner {
  static constexpr std::string_view _eo_name = "main.Inner";

  int64_t A{};
  std::string B{};

  static constexpr eo::field _eo_fields[] = {{.tag = "json:\"a\""}, {.tag = "json:\"b,omitempty\""}};

  auto operator==(const Inner&) const -> bool = default;
};

struct Embedded {
  static constexpr std::string_view _eo_name = "main.Embedded";

  int64_t E1{};
  std::string E2{};

  static constexpr eo::field _eo_fields[] = {{}, {.tag = "json:\"e2\""}};

  auto operator==(const Embedded&) const -> bool = default;
};

struct Hex : eo::defined<Hex, uint64_t> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Hex";

  static auto MarshalText(Hex h) -> std::tuple<slice<uint8_t>, error>;
  static auto UnmarshalText(ptr<Hex> h, slice<uint8_t> b) -> error;

  using _eo_methods =
    eo::methods<eo::method<&Hex::MarshalText, "MarshalText">, eo::method<&Hex::UnmarshalText, "UnmarshalText">>;
};

struct Wrapped {
  static constexpr std::string_view _eo_name = "main.Wrapped";

  int64_t V{};

  auto operator==(const Wrapped&) const -> bool = default;

  static auto MarshalJSON(Wrapped w) -> std::tuple<slice<uint8_t>, error>;
  static auto UnmarshalJSON(ptr<Wrapped> w, slice<uint8_t> b) -> error;

  using _eo_methods =
    eo::methods<eo::method<&Wrapped::MarshalJSON, "MarshalJSON">, eo::method<&Wrapped::UnmarshalJSON, "UnmarshalJSON">>;
};

struct wrappedJSON {
  static constexpr std::string_view _eo_name = "main.wrappedJSON";

  int64_t Wrapped{};

  static constexpr eo::field _eo_fields[] = {{.tag = "json:\"wrapped\""}};

  auto operator==(const wrappedJSON&) const -> bool = default;
};

struct Config {
  static constexpr std::string_view _eo_name = "main.Config";

  std::string Name{};
  int64_t Count{};
  int64_t ID{};
  double Ratio{};
  float Small{};
  bool Enabled{};
  slice<std::string> Tags{};
  map<std::string, int64_t> Attrs{};
  map<int64_t, std::string> ByID{};
  json::RawMessage Raw{};
  json::Number Num{};
  slice<uint8_t> Data{};
  ptr<::Inner> Ptr{};
  ::Inner Inner{};
  ptr<::Embedded> Embedded{};
  ::Hex Hex{};
  map<::Hex, bool> HexKeys{};
  ::Wrapped Wrapped{};
  any Any{};
  int64_t Skip{};
  int64_t hidden{};
  std::array<uint8_t, 3> Arr{};
  ::Inner Zero{};

  static constexpr eo::field _eo_fields[] = {{.tag = "json:\"name\""},
    {.tag = "json:\"count,omitempty\""},
    {.tag = "json:\"id,string\""},
    {.tag = "json:\"ratio\""},
    {.tag = "json:\"small\""},
    {.tag = "json:\"enabled\""},
    {.tag = "json:\"tags\""},
    {.tag = "json:\"attrs,omitempty\""},
    {.tag = "json:\"by_id\""},
    {.tag = "json:\"raw\""},
    {.tag = "json:\"num\""},
    {.tag = "json:\"data\""},
    {.tag = "json:\"ptr\""},
    {.embedded = true},
    {.embedded = true},
    {.tag = "json:\"hex\""},
    {.tag = "json:\"hex_keys\""},
    {.tag = "json:\"wrapped\""},
    {.tag = "json:\"any\""},
    {.tag = "json:\"-\""},
    {},
    {.tag = "json:\"arr\""},
    {.tag = "json:\"zero,omitzero\""}};

  auto operator==(const Config&) const -> bool = default;
};

struct PtrMarshal {
  static constexpr std::string_view _eo_name = "main.PtrMarshal";

  int64_t N{};

  auto operator==(const PtrMarshal&) const -> bool = default;

  static auto MarshalJSON(ptr<PtrMarshal> p) -> std::tuple<slice<uint8_t>, error>;

  using _eo_methods = eo::methods<eo::method<&PtrMarshal::MarshalJSON, "MarshalJSON">>;
};

struct Holder {
  static constexpr std::string_view _eo_name = "main.Holder";

  PtrMarshal P{};

  auto operator==(const Holder&) const -> bool = default;
};

struct Bad {
  static constexpr std::string_view _eo_name = "main.Bad";

  auto operator==(const Bad&) const -> bool = default;

  static auto MarshalJSON(Bad) -> std::tuple<slice<uint8_t>, error>;

  using _eo_methods = eo::methods<eo::method<&Bad::MarshalJSON, "MarshalJSON">>;
};

struct Node {
  static constexpr std::string_view _eo_name = "main.Node";

  std::string Name{};
  ptr<Node> Next{};

  auto operator==(const Node&) const -> bool = default;
};

struct Small {
  static constexpr std::string_view _eo_name = "main.Small";

  int8_t N{};
  uint8_t U{};
  float F{};
  std::string S{};

  auto operator==(const Small&) const -> bool = default;
};

struct Tree {
  static constexpr std::string_view _eo_name = "main.Tree";

  int64_t Value{};
  slice<ptr<Tree>> Children{};

  static constexpr eo::field _eo_fields[] = {{.tag = "json:\"value\""}, {.tag = "json:\"children,omitempty\""}};

  auto operator==(const Tree&) const -> bool = default;
};

auto Hex::MarshalText(Hex h) -> std::tuple<slice<uint8_t>, error> {
  return {eo::bytes_from_string("0x"s + strconv::FormatUint(uint64_t(h), 16)), nullptr};
}

auto Hex::UnmarshalText(ptr<Hex> h, slice<uint8_t> b) -> error {
  auto s = eo::string_from_bytes(b);
  if (!strings::HasPrefix(s, "0x"s)) {
    return errors::New("hex: missing 0x prefix"s);
  }
  auto [v, err] = strconv::ParseUint(eo::substr(s, 2), 16, 64);
  if (err != nullptr) {
    return err;
  }
  *h = Hex(v);
  return nullptr;
}

auto Wrapped::MarshalJSON(Wrapped w) -> std::tuple<slice<uint8_t>, error> {
  return {eo::bytes_from_string(fmt::Sprintf(R"({ "wrapped" : %d })"s, w.V)), nullptr};
}

auto Wrapped::UnmarshalJSON(ptr<Wrapped> w, slice<uint8_t> b) -> error {
  auto x = New<wrappedJSON>();
  if (auto err = json::Unmarshal(b, x); err != nullptr) {
    return err;
  }
  w->V = x->Wrapped;
  return nullptr;
}

auto PtrMarshal::MarshalJSON(ptr<PtrMarshal> p) -> std::tuple<slice<uint8_t>, error> {
  return {eo::bytes_from_string(strconv::Itoa(p->N * 100)), nullptr};
}

auto Bad::MarshalJSON(Bad) -> std::tuple<slice<uint8_t>, error> {
  return {nullptr, errors::New("bad"s)};
}

auto newConfig() -> Config {
  return Config{
    .Name = "eo <&> \u2028"s,
    .ID = int64_t{9007199254740992},
    .Ratio = double{0x1.999999999999ap-04},
    .Small = float{0x1.91eb86p+01f},
    .Enabled = true,
    .Tags = slice<std::string>::of({"x"s, "y"s}),
    .ByID = map<int64_t, std::string>::of({{2, "two"s}, {-1, "minus one"s}, {10, "ten"s}}),
    .Raw = json::RawMessage(eo::bytes_from_string(R"({"k": [1, 2]})"s)),
    .Num = json::Number{"12.50"s},
    .Data = eo::bytes_from_string("hello\x00"s),
    .Inner = ::Inner{.A = 7},
    .Embedded = New<::Embedded>(::Embedded{.E1 = 8, .E2 = "e"s}),
    .Hex = ::Hex{255},
    .HexKeys = map<::Hex, bool>::of({{::Hex{16}, true}, {::Hex{1}, false}}),
    .Wrapped = ::Wrapped{.V = 3},
    .Any = slice<any>::of({int64_t{1}, "s"s, nullptr, map<std::string, any>::of({{"z"s, double{1.5}}})}),
    .Skip = 9,
    .hidden = 10,
    .Arr = std::array<uint8_t, 3>{1, 2, 3},
  };
}

void marshalling() {
  auto c = newConfig();
  auto [b, err] = json::Marshal(c);
  fmt::Println(eo::string_from_bytes(b), err);

  std::tie(b, err) = json::MarshalIndent(Inner{.A = 1, .B = "b"s}, ">"s, "  "s);
  fmt::Println(eo::string_from_bytes(b), err);

  auto h = New<Holder>(Holder{.P = PtrMarshal{.N = 4}});
  std::tie(b, err) = json::Marshal(*h);
  fmt::Println(eo::string_from_bytes(b), err);
  std::tie(b, err) = json::Marshal(h);
  fmt::Println(eo::string_from_bytes(b), err);

  auto _eo_call_0 = math::Copysign(double{0}, double{-1});
  for (auto [_eo_range_key, v] : eo::range(slice<any>::of({nullptr,
         true,
         int64_t{42},
         int64_t{-7},
         uint8_t{200},
         double{1e+21},
         double{1e+20},
         double{0x1.ad7f29abcaf48p-24},
         double{0x1.0c6f7a0b5ed8dp-20},
         float{0x1.ad7f2ap-24f},
         _eo_call_0,
         double{1.23456789e+08},
         "tab\tquote\"back\\slash\x01"s,
         "bad \xff utf8"s,
         "emoji 😀"s,
         slice<int64_t>{},
         slice<int64_t>::of({}),
         map<std::string, int64_t>{},
         New<Inner>(Inner{.A = 2}),
         ptr<Inner>{},
         slice<uint8_t>::of({}),
         std::array<int64_t, 0>{},
         json::Number{""s},
         Wrapped{.V = 5},
         New<Wrapped>(Wrapped{.V = 6}),
         Hex{10}}))) {
    auto [b, err] = json::Marshal(v);
    fmt::Printf("%s %v\n"s, b, err);
  }
}

void marshalErrors() {
  auto _eo_call_0 = math::NaN();
  auto _eo_call_1 = math::Inf(-1);
  for (auto [_eo_range_key, v] : eo::range(slice<any>::of({_eo_call_0,
         _eo_call_1,
         map<Inner, int64_t>::of({{Inner{.A = 1}, 1}}),
         Bad{},
         slice<Bad>::of({Bad{}}),
         json::Number{"1.2.3"s}}))) {
    auto [b, err] = json::Marshal(v);
    fmt::Printf("%q %v\n"s, b, err);
  }

  auto n = New<Node>(Node{.Name = "loop"s});
  n->Next = n;
  auto err = std::get<1>(json::Marshal(n));
  fmt::Println(err);
  auto ue = New<ptr<json::UnsupportedValueError>>();
  fmt::Println(errors::As(err, ue));
}

void unmarshalling() {
  auto c = New<Config>();
  auto in = R"({"NAME":"n","count":3,"id":"42","ratio":2.5,"small":1.25,"enabled":true,"tags":["a"],
		"attrs":{"k":1},"by_id":{"5":"five"},"raw":[ 1 , 2 ],"num":7,"data":"aGk=","ptr":{"a":9},
		"a":4,"b":"bee","E1":5,"e2":"ee","hex":"0x1f","hex_keys":{"0x2":true},"wrapped":{"wrapped":11},
		"any":{"list":[1,"two",false,null],"f":-1.5e3},"Skip":1,"private":2,"arr":[9],"unknown":0})"s;
  auto err = json::Unmarshal(eo::bytes_from_string(in), c);
  fmt::Println(err);
  fmt::Println(c->Name, c->Count, c->ID, c->Ratio, c->Small, c->Enabled, c->Tags, c->Attrs, c->ByID);
  fmt::Println(eo::string_from_bytes(c->Raw),
    c->Num,
    eo::string_from_bytes(c->Data),
    *c->Ptr,
    c->Inner,
    *c->Embedded,
    c->Hex,
    c->HexKeys,
    c->Wrapped);
  fmt::Println(c->Any, c->Skip, c->hidden, c->Arr);

  auto m = New<map<std::string, any>>();
  err = json::Unmarshal(eo::bytes_from_string(R"( {"b":[true,{"c":null}],"a":"\u00e9\ud83d\ude00\n","n":1e400} )"s), m);
  fmt::Println(*m, err);
  err = json::Unmarshal(eo::bytes_from_string(R"({"b":[true,{"c":null}],"a":"\u00e9\ud83d\ude00\n\/"})"s), m);
  fmt::Println(*m, err);

  auto s = New<slice<int64_t>>(slice<int64_t>::of({1, 2, 3, 4}));
  err = json::Unmarshal(eo::bytes_from_string("[5,6]"s), s);
  fmt::Println(*s, eo::len(*s), err);
  auto arr = New<std::array<int64_t, 3>>(std::array<int64_t, 3>{1, 2, 3});
  err = json::Unmarshal(eo::bytes_from_string("[7]"s), arr);
  fmt::Println(*arr, err);
  auto p = New<ptr<Inner>>(New<Inner>(Inner{.A = 1}));
  err = json::Unmarshal(eo::bytes_from_string("null"s), p);
  fmt::Println(*p == nullptr, err);
  auto kept = New<Inner>(Inner{.A = 5, .B = "keep"s});
  err = json::Unmarshal(eo::bytes_from_string(R"({"a":null,"b":null})"s), kept);
  fmt::Println(*kept, err);
  auto x = New<any>(New<Inner>(Inner{}));
  err = json::Unmarshal(eo::bytes_from_string(R"({"a":3})"s), x);
  fmt::Println(*eo::type_assert<ptr<Inner>>(*x), err);
  auto r = New<json::RawMessage>();
  err = json::Unmarshal(eo::bytes_from_string(R"( [1, {"a" : 2}] )"s), r);
  fmt::Println(eo::string_from_bytes(*r), err);
  auto n = New<json::Number>();
  err = json::Unmarshal(eo::bytes_from_string(R"("-12e3")"s), n);
  auto [i, ierr] = json::Number::Int64(*n);
  auto [f, ferr] = json::Number::Float64(*n);
  fmt::Println(*n, i, ierr, f, ferr);
}

void unmarshalErrors() {
  auto c = New<Config>();
  auto sm = New<Small>();
  auto i = New<int64_t>();
  for (auto [_eo_range_key, in] : eo::range(slice<std::string>::of({R"({"name":1})"s,
         R"({"N":300})"s,
         R"({"U":-1})"s,
         R"({"F":1e40})"s,
         R"({"S":[1]})"s,
         R"({"N":"1"})"s,
         R"({"by_id":{"x":"y"}})"s,
         R"({"hex":"zz"})"s,
         R"({"hex":12})"s,
         R"({"id":"abc"})"s,
         R"({"id":12})"s,
         R"({"ptr":{"a":"s"}})"s,
         R"({"a":1,})"s,
         "[1,2"s,
         R"({"a" 1})"s,
         "tru"s,
         "01"s,
         R"("\x")"s,
         ""s,
         "  "s,
         "{}x"s,
         "1 2"s}))) {
    error err{};
    if (strings::Contains(in, R"("N")"s) || strings::Contains(in, R"("U")"s) || strings::Contains(in, R"("F")"s) ||
      strings::Contains(in, R"("S")"s)) {
      err = json::Unmarshal(eo::bytes_from_string(in), sm);
    } else if (strings::HasPrefix(in, "{"s)) {
      err = json::Unmarshal(eo::bytes_from_string(in), c);
    } else {
      err = json::Unmarshal(eo::bytes_from_string(in), i);
    }
    fmt::Printf("%q: %v\n"s, in, err);
    if (auto [se, ok] = eo::type_assert_ok<ptr<json::SyntaxError>>(err); ok) {
      fmt::Println("  offset"s, se->Offset);
    }
    if (auto [te, ok] = eo::type_assert_ok<ptr<json::UnmarshalTypeError>>(err); ok) {
      fmt::Println("  type"s, te->Value, te->Type, te->Offset, te->Struct, te->Field);
    }
  }
  any none = nullptr;
  any value = *i;
  ptr<int64_t> np{};
  for (auto [_eo_range_key, target] : eo::range(slice<any>::of({none, value, np}))) {
    fmt::Println(json::Unmarshal(eo::bytes_from_string("1"s), target));
  }
}

void helpers() {
  auto _eo_call_0 = json::Valid(eo::bytes_from_string(R"({"a":[1,2,{"b":null}]})"s));
  auto _eo_call_1 = json::Valid(eo::bytes_from_string(R"({"a":})"s));
  auto _eo_call_2 = json::Valid(nullptr);
  fmt::Println(_eo_call_0, _eo_call_1, _eo_call_2);

  bytes::Buffer buf{};
  auto err = json::Compact(ref(buf), eo::bytes_from_string(" { \"a\" : [ 1 , 2 ] ,\n \"b\" : \"<x>\" } "s));
  fmt::Println(bytes::Buffer::String(ref(buf)), err);
  bytes::Buffer::Reset(ref(buf));
  err = json::Indent(ref(buf), eo::bytes_from_string(R"({"a":[1,2,{}],"b":[],"c":{"d":"e"}})"s), ""s, "\t"s);
  fmt::Println(bytes::Buffer::String(ref(buf)), err);
  bytes::Buffer::Reset(ref(buf));
  err = json::Indent(ref(buf), eo::bytes_from_string(R"({"a":[1,2)"s), ""s, "\t"s);
  fmt::Printf("%q %v\n"s, bytes::Buffer::String(ref(buf)), err);
  bytes::Buffer::Reset(ref(buf));
  json::HTMLEscape(ref(buf), eo::bytes_from_string(R"({"h":"<b>&amp;</b>)"s + "\u2029"s + R"("})"s));
  fmt::Println(bytes::Buffer::String(ref(buf)));
}

void streams() {
  auto buf = New<bytes::Buffer>();
  auto enc = json::NewEncoder(buf);
  fmt::Println(json::Encoder::Encode(enc, map<std::string, any>::of({{"html"s, "<&>"s}, {"n"s, int64_t{1}}})));
  json::Encoder::SetEscapeHTML(enc, false);
  json::Encoder::SetIndent(enc, ""s, " "s);
  fmt::Println(
    json::Encoder::Encode(enc, map<std::string, any>::of({{"html"s, "<&>"s}, {"list"s, slice<int64_t>::of({1, 2})}})));
  fmt::Println(json::Encoder::Encode(enc, math::NaN()));
  fmt::Print(bytes::Buffer::String(buf));

  auto dec = json::NewDecoder(strings::NewReader(R"( {"a":1} [2,3] "four" 5.5 null {"b":)"s));
  for (;;) {
    auto v = New<any>();
    auto err = json::Decoder::Decode(dec, v);
    fmt::Println(*v, err, json::Decoder::InputOffset(dec));
    if (err != nullptr) {
      break;
    }
  }

  dec = json::NewDecoder(strings::NewReader(R"({"x":1.0,"y":[10,20]})"s));
  json::Decoder::UseNumber(dec);
  auto v = New<map<std::string, any>>();
  auto err = json::Decoder::Decode(dec, v);
  fmt::Println(err, *v);
  fmt::Printf("%T\n"s, eo::map_get(*v, "x"s));

  dec = json::NewDecoder(strings::NewReader(R"({"a":1,"zzz":2})"s));
  json::Decoder::DisallowUnknownFields(dec);
  auto in = New<Inner>();
  err = json::Decoder::Decode(dec, in);
  fmt::Println(err, *in);

  dec = json::NewDecoder(strings::NewReader(R"({"k":[1,"s",{"n":null}],"t":true} rest)"s));
  for (;;) {
    auto [tok, err] = json::Decoder::Token(dec);
    if (err != nullptr) {
      fmt::Println("token error:"s, err);
      break;
    }
    fmt::Printf("%T %v more=%v\n"s, tok, tok, json::Decoder::More(dec));
  }

  dec = json::NewDecoder(strings::NewReader(R"([{"a":1},{"a":2},{"a":3}])"s));
  json::Token tok{};
  std::tie(tok, err) = json::Decoder::Token(dec);
  fmt::Println(tok, err);
  while (json::Decoder::More(dec)) {
    auto e = New<Inner>();
    auto err = json::Decoder::Decode(dec, e);
    fmt::Println(*e, err);
  }
  std::tie(tok, err) = json::Decoder::Token(dec);
  fmt::Println(tok, err);
  std::tie(tok, err) = json::Decoder::Token(dec);
  fmt::Println(tok, err);

  dec = json::NewDecoder(strings::NewReader(R"({"a":1} tail)"s));
  auto first = New<map<std::string, int64_t>>();
  err = json::Decoder::Decode(dec, first);
  fmt::Println(err, *first);
  auto rest = New<bytes::Buffer>();
  bytes::Buffer::ReadFrom(rest, json::Decoder::Buffered(dec));
  fmt::Printf("%q\n"s, bytes::Buffer::String(rest));
}

func<> concurrent() {
  auto wg = New<sync::WaitGroup>();
  auto results = eo::make_slice<std::string>(8);
  for (auto g = int64_t{0}; g < 8; g++) {
    wg->Add(1);
    go([=]() mutable -> func<> {
      eo_defer_scope;
      try {
        auto _eo_defer_receiver_0 = wg;
        eo_defer([=] { _eo_defer_receiver_0->Done(); });
        auto [b, err] = json::Marshal(
          New<Tree>(Tree{.Value = g, .Children = slice<ptr<Tree>>::of({New<Tree>(Tree{.Value = g * 10})})}));
        if (err != nullptr) {
          panic(err);
        }
        eo::at(results, g) = eo::string_from_bytes(b);
        eo_defer_run;
      } catch (...) {
        eo_defer_panic;
      }
      co_return;
    });
  }
  co_await wg->Wait();
  fmt::Println(results);
}

func<> eo_main() {
  marshalling();
  marshalErrors();
  unmarshalling();
  unmarshalErrors();
  helpers();
  streams();
  co_await concurrent();
  co_return;
}
