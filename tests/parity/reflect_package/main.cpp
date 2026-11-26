#include <eo/errors.h>
#include <eo/fmt.h>
#include <eo/reflect.h>
#include <eo/slices.h>

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>

using namespace eo;

struct Point;
struct Base;
struct Tagged;
struct Celsius;
struct Bytes;
struct Outer;
struct Stringer;
struct inner;
struct Wrapper;

struct Point {
  static constexpr std::string_view _eo_name = "main.Point";

  int64_t X{}, Y{};

  auto operator==(const Point&) const -> bool = default;
};

struct Base {
  static constexpr std::string_view _eo_name = "main.Base";

  int64_t ID{};

  auto operator==(const Base&) const -> bool = default;

  static auto Describe(Base b) -> std::string;

  using _eo_methods = eo::methods<eo::method<&Base::Describe, "Describe">>;
};

struct Tagged {
  static constexpr std::string_view _eo_name = "main.Tagged";

  std::string Name{};
  int64_t Count{};
  bool hidden{};
  ::Base Base{};
  ptr<Tagged> Next{};
  slice<std::string> Tags{};
  map<std::string, int64_t> Attrs{};
  error Err{};
  any Any{};
  std::array<uint8_t, 4> Raw{};
  double F{};

  static constexpr eo::field _eo_fields[] = {{.tag = "json:\"name\" xml:\"n\""},
    {.tag = "json:\"count,omitempty\""},
    {},
    {.embedded = true},
    {},
    {},
    {},
    {},
    {},
    {},
    {}};

  auto operator==(const Tagged&) const -> bool = default;

  static auto Describe(Tagged _eo_receiver) -> std::string;

  using _eo_methods = eo::methods<eo::method<&Tagged::Describe, "Describe">>;
};

struct Celsius : eo::defined<Celsius, double> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Celsius";

  static auto String(Celsius c) -> std::string;

  using _eo_methods = eo::methods<eo::method<&Celsius::String, "String">>;
};

struct Bytes : eo::defined<Bytes, slice<uint8_t>> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Bytes";
};

struct Outer {
  static constexpr std::string_view _eo_name = "main.Outer";

  ptr<::Point> Point{};

  static constexpr eo::field _eo_fields[] = {{.embedded = true}};

  auto operator==(const Outer&) const -> bool = default;
};

struct Stringer : eo::interface<Stringer> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "main.Stringer";

  auto String() const -> std::string;

  using _eo_methods = eo::methods<eo::method<&Stringer::String, "String">>;
};

inline auto Stringer::String() const -> std::string {
  return _eo_call<&Stringer::String>();
}

struct inner {
  static constexpr std::string_view _eo_name = "main.inner";

  int64_t Val{};
  std::string note{};

  auto operator==(const inner&) const -> bool = default;
};

struct Wrapper {
  static constexpr std::string_view _eo_name = "main.Wrapper";

  ::inner inner{};
  chan<int64_t> Ch{};

  static constexpr eo::field _eo_fields[] = {{.embedded = true}, {}};

  auto operator==(const Wrapper&) const -> bool = default;
};

auto Base::Describe(Base b) -> std::string {
  return fmt::Sprintf("base %d"s, b.ID);
}

auto Tagged::Describe(Tagged _eo_receiver) -> std::string {
  return ::Base::Describe(_eo_receiver.Base);
}

auto Celsius::String(Celsius c) -> std::string {
  return fmt::Sprintf("%.1fC"s, double(c));
}

void kinds() {
  for (auto [_eo_range_key, k] : eo::range(slice<reflect::Kind>::of({reflect::Invalid,
         reflect::Bool,
         reflect::Int,
         reflect::Uint8,
         reflect::Float64,
         reflect::Array,
         reflect::Interface,
         reflect::Map,
         reflect::Pointer,
         reflect::Slice,
         reflect::String,
         reflect::Struct,
         reflect::UnsafePointer,
         reflect::Kind{99u}}))) {
    fmt::Println(int64_t(k), reflect::Kind::String(k), k);
  }
  fmt::Println(reflect::Ptr == reflect::Pointer);
}

void types() {
  auto values = slice<any>::of({true,
    int64_t{1},
    int8_t{2},
    int16_t{3},
    int32_t{4},
    uint64_t{6u},
    uint8_t{7u},
    uint16_t{8u},
    uint32_t{9u},
    float{1.5f},
    double{2.5},
    "s"s,
    slice<int64_t>::of({1}),
    std::array<std::string, 2>{},
    map<std::string, bool>::of({}),
    New<Point>(Point{}),
    Point{},
    Celsius{1.0},
    Bytes{slice<uint8_t>::of({1})},
    empty_struct{}});
  for (auto [_eo_range_key, v] : eo::range(values)) {
    auto t = reflect::TypeOf(v);
    auto _eo_call_0 = t.String();
    auto _eo_call_1 = t.Name();
    auto _eo_call_2 = t.Kind();
    auto _eo_call_3 = t.Comparable();
    fmt::Printf("%s|%s|%s|%v\n"s, _eo_call_0, _eo_call_1, _eo_call_2, _eo_call_3);
  }
  fmt::Println(reflect::TypeOf(nullptr) == nullptr);
  auto _eo_call_4 = reflect::TypeOf(slice<int64_t>::of({}));
  auto _eo_call_5 = _eo_call_4.Elem();
  auto _eo_call_6 = reflect::TypeOf(map<std::string, bool>::of({}));
  auto _eo_call_7 = _eo_call_6.Key();
  auto _eo_call_8 = reflect::TypeOf(map<std::string, bool>::of({}));
  auto _eo_call_9 = _eo_call_8.Elem();
  auto _eo_call_10 = reflect::TypeOf(std::array<int64_t, 3>{});
  auto _eo_call_11 = _eo_call_10.Len();
  auto _eo_call_12 = reflect::TypeOf(New<Point>(Point{}));
  auto _eo_call_13 = _eo_call_12.Elem();
  auto _eo_call_14 = _eo_call_13.Name();
  fmt::Println(_eo_call_5, _eo_call_7, _eo_call_9, _eo_call_11, _eo_call_14);
  auto _eo_call_15 = reflect::TypeOf(int8_t{0});
  auto _eo_call_16 = _eo_call_15.Bits();
  auto _eo_call_17 = reflect::TypeOf(float{0.0f});
  auto _eo_call_18 = _eo_call_17.Bits();
  auto _eo_call_19 = reflect::TypeOf(int64_t{0});
  auto _eo_call_20 = _eo_call_19.Bits();
  fmt::Println(_eo_call_16, _eo_call_18, _eo_call_20);

  auto errType = reflect::TypeFor<error>();
  auto anyType = reflect::TypeFor<any>();
  auto stringer = reflect::TypeFor<Stringer>();
  auto _eo_call_21 = errType.Kind();
  auto _eo_call_22 = stringer.Name();
  auto _eo_call_23 = anyType.Name();
  fmt::Printf("%s|%s|%s|%s|%q|%s\n"s, errType, _eo_call_21, stringer, _eo_call_22, _eo_call_23, anyType);
  auto _eo_call_24 = reflect::TypeOf(Celsius{0.0});
  auto _eo_call_25 = _eo_call_24.Implements(stringer);
  auto _eo_call_26 = reflect::TypeOf(Point{});
  auto _eo_call_27 = _eo_call_26.Implements(stringer);
  auto _eo_call_28 = errType.Implements(anyType);
  auto _eo_call_29 = stringer.Implements(errType);
  fmt::Println(_eo_call_25, _eo_call_27, _eo_call_28, _eo_call_29);
  auto _eo_call_30 = errors::New("x"s);
  auto _eo_call_31 = reflect::TypeOf(_eo_call_30);
  auto _eo_call_32 = _eo_call_31.Implements(errType);
  auto _eo_call_33 = errors::New("x"s);
  auto _eo_call_34 = reflect::TypeOf(_eo_call_33);
  fmt::Println(_eo_call_32, _eo_call_34);
  auto _eo_call_35 = reflect::TypeOf(Celsius{0.0});
  auto _eo_call_36 = reflect::PointerTo(_eo_call_35);
  auto _eo_call_37 = _eo_call_36.Implements(stringer);
  auto _eo_call_38 = reflect::TypeOf(Point{});
  auto _eo_call_39 = reflect::PointerTo(_eo_call_38);
  auto _eo_call_40 = reflect::TypeOf(New<Point>(Point{}));
  auto _eo_call_41 = reflect::TypeOf(int64_t{0});
  auto _eo_call_42 = reflect::PtrTo(_eo_call_41);
  fmt::Println(_eo_call_37, _eo_call_39 == _eo_call_40, _eo_call_42);
  auto _eo_call_43 = reflect::TypeOf(Point{});
  auto _eo_call_44 = _eo_call_43.NumMethod();
  auto _eo_call_45 = reflect::TypeOf(Base{});
  auto _eo_call_46 = _eo_call_45.NumMethod();
  auto _eo_call_47 = reflect::TypeOf(Tagged{});
  auto _eo_call_48 = _eo_call_47.NumMethod();
  auto _eo_call_49 = stringer.NumMethod();
  auto _eo_call_50 = anyType.NumMethod();
  fmt::Println(_eo_call_44, _eo_call_46, _eo_call_48, _eo_call_49, _eo_call_50);
  auto _eo_call_51 = reflect::TypeOf(Bytes{slice<uint8_t>::of({})});
  auto _eo_call_52 = reflect::TypeOf(slice<uint8_t>::of({}));
  auto _eo_call_53 = _eo_call_51.AssignableTo(_eo_call_52);
  auto _eo_call_54 = reflect::TypeOf(slice<uint8_t>::of({}));
  auto _eo_call_55 = reflect::TypeOf(Bytes{slice<uint8_t>::of({})});
  auto _eo_call_56 = _eo_call_54.AssignableTo(_eo_call_55);
  auto _eo_call_57 = reflect::TypeOf(Celsius{0.0});
  auto _eo_call_58 = reflect::TypeOf(double{0.0});
  auto _eo_call_59 = _eo_call_57.AssignableTo(_eo_call_58);
  auto _eo_call_60 = reflect::TypeOf(Point{});
  auto _eo_call_61 = _eo_call_60.AssignableTo(anyType);
  fmt::Println(_eo_call_53, _eo_call_56, _eo_call_59, _eo_call_61);
  auto _eo_call_62 = reflect::TypeOf(Tagged{});
  auto _eo_call_63 = _eo_call_62.Comparable();
  auto _eo_call_64 = reflect::TypeOf(std::array<Point, 2>{});
  auto _eo_call_65 = _eo_call_64.Comparable();
  auto _eo_call_66 = errType.Comparable();
  fmt::Println(_eo_call_63, _eo_call_65, _eo_call_66);
  auto pp = reflect::PointerTo(reflect::TypeOf(New<Point>(Point{})));
  auto _eo_call_67 = pp.Elem();
  auto _eo_call_68 = reflect::TypeOf(New<Point>(Point{}));
  auto _eo_call_69 = pp.Elem();
  auto _eo_call_70 = _eo_call_69.Elem();
  auto _eo_call_71 = reflect::TypeOf(Point{});
  auto _eo_call_72 = reflect::TypeOf(Celsius{0.0});
  auto _eo_call_73 = reflect::PointerTo(_eo_call_72);
  auto _eo_call_74 = reflect::TypeOf(New<Celsius>());
  fmt::Println(pp, _eo_call_67 == _eo_call_68, _eo_call_70 == _eo_call_71, _eo_call_73 == _eo_call_74);
}

void structFields() {
  auto t = reflect::TypeOf(Tagged{});
  fmt::Println(t.NumField());
  for (auto i = int64_t{0}; i < t.NumField(); i++) {
    auto f = t.Field(i);
    auto _eo_call_0 = reflect::StructField::IsExported(f);
    auto _eo_call_1 = reflect::StructTag::Get(f.Tag, "json"s);
    fmt::Printf("%d %s %q %s exported=%v anonymous=%v index=%v json=%q\n"s,
      i,
      f.Name,
      f.PkgPath,
      f.Type,
      _eo_call_0,
      f.Anonymous,
      f.Index,
      _eo_call_1);
  }
  auto [v, ok] = reflect::StructTag::Lookup(t.Field(0).Tag, "xml"s);
  fmt::Println(v, ok);
  std::tie(v, ok) = reflect::StructTag::Lookup(t.Field(0).Tag, "yaml"s);
  fmt::Printf("%q %v\n"s, v, ok);
  fmt::Println(reflect::StructTag::Get(reflect::StructTag(R"(a:"1"  b:"two \"quoted\"")"s), "b"s));
  std::tie(v, ok) = reflect::StructTag::Lookup(reflect::StructTag("bad"s), "bad"s);
  fmt::Printf("%q %v\n"s, v, ok);
  fmt::Println(reflect::StructTag::Get(reflect::StructTag(R"(a:"\x41\u00e9")"s), "a"s));
}

void values() {
  auto p = New<Point>(Point{1, 2});
  auto v = reflect::ValueOf(p);
  auto _eo_call_0 = reflect::Value::Kind(v);
  auto _eo_call_1 = reflect::Value::Type(v);
  auto _eo_call_2 = reflect::Value::CanAddr(v);
  auto _eo_call_3 = reflect::Value::CanSet(v);
  fmt::Println(_eo_call_0, _eo_call_1, _eo_call_2, _eo_call_3);
  auto e = reflect::Value::Elem(v);
  auto _eo_call_4 = reflect::Value::Kind(e);
  auto _eo_call_5 = reflect::Value::Type(e);
  auto _eo_call_6 = reflect::Value::CanAddr(e);
  auto _eo_call_7 = reflect::Value::CanSet(e);
  auto _eo_call_8 = reflect::Value::NumField(e);
  fmt::Println(_eo_call_4, _eo_call_5, _eo_call_6, _eo_call_7, _eo_call_8);
  reflect::Value::SetInt(reflect::Value::Field(e, 0), 10);
  reflect::Value::SetInt(reflect::Value::FieldByIndex(e, slice<int64_t>::of({1})), 20);
  auto _eo_call_9 = reflect::Value::Field(e, 0);
  auto _eo_call_10 = reflect::Value::Int(_eo_call_9);
  auto _eo_call_11 = reflect::Value::Field(e, 1);
  auto _eo_call_12 = reflect::Value::Interface(_eo_call_11);
  fmt::Println(*p, _eo_call_10, _eo_call_12);
  auto w = reflect::ValueOf(Point{3, 4});
  auto _eo_call_13 = reflect::Value::CanAddr(w);
  auto _eo_call_14 = reflect::Value::Field(w, 0);
  auto _eo_call_15 = reflect::Value::CanSet(_eo_call_14);
  auto _eo_call_16 = reflect::Value::Field(w, 1);
  auto _eo_call_17 = reflect::Value::Int(_eo_call_16);
  fmt::Println(_eo_call_13, _eo_call_15, _eo_call_17);
  auto _eo_call_18 = reflect::ValueOf(nullptr);
  auto _eo_call_19 = reflect::Value::IsValid(_eo_call_18);
  auto _eo_call_20 = reflect::ValueOf(int64_t{0});
  auto _eo_call_21 = reflect::Value::IsValid(_eo_call_20);
  fmt::Println(_eo_call_19, _eo_call_21);
  auto _eo_call_22 = reflect::ValueOf("hi"s);
  auto _eo_call_23 = reflect::Value::String(_eo_call_22);
  auto _eo_call_24 = reflect::ValueOf(int64_t{3});
  auto _eo_call_25 = reflect::Value::String(_eo_call_24);
  auto _eo_call_26 = reflect::Value::String(reflect::Value{});
  fmt::Println(_eo_call_23, _eo_call_25, _eo_call_26);
  auto _eo_call_27 = reflect::ValueOf(true);
  auto _eo_call_28 = reflect::Value::Bool(_eo_call_27);
  auto _eo_call_29 = reflect::ValueOf(uint8_t{200u});
  auto _eo_call_30 = reflect::Value::Uint(_eo_call_29);
  auto _eo_call_31 = reflect::ValueOf(float{1.25f});
  auto _eo_call_32 = reflect::Value::Float(_eo_call_31);
  auto _eo_call_33 = reflect::ValueOf(int16_t{-3});
  auto _eo_call_34 = reflect::Value::Int(_eo_call_33);
  fmt::Println(_eo_call_28, _eo_call_30, _eo_call_32, _eo_call_34);
  auto _eo_call_35 = reflect::ValueOf(Celsius{2.5});
  auto _eo_call_36 = reflect::Value::Float(_eo_call_35);
  auto _eo_call_37 = reflect::ValueOf(Celsius{2.5});
  auto _eo_call_38 = reflect::Value::Kind(_eo_call_37);
  fmt::Println(_eo_call_36, _eo_call_38);
  fmt::Println(eo::type_assert<ptr<Point>>(reflect::Value::Interface(reflect::ValueOf(p))) == p);
  auto _eo_call_39 = reflect::ValueOf(Point{5, 6});
  auto _eo_call_40 = reflect::ValueOf(Celsius{1.0});
  fmt::Println(_eo_call_39, _eo_call_40);
  fmt::Println(reflect::Value{});

  auto a = reflect::Value::Addr(reflect::Value::Field(e, 0));
  auto _eo_call_41 = reflect::Value::Type(a);
  auto _eo_call_42 = reflect::Value::Interface(a);
  fmt::Println(_eo_call_41, *eo::type_assert<ptr<int64_t>>(_eo_call_42));
  *eo::type_assert<ptr<int64_t>>(reflect::Value::Interface(a)) = 99;
  fmt::Println(p->X);
  auto _eo_call_43 = reflect::Indirect(v);
  auto _eo_call_44 = reflect::Value::Field(_eo_call_43, 0);
  auto _eo_call_45 = reflect::ValueOf(int64_t{7});
  auto _eo_call_46 = reflect::Indirect(_eo_call_45);
  fmt::Println(_eo_call_44, _eo_call_46);

  auto n = reflect::New(reflect::TypeOf(Point{}));
  reflect::Value::SetInt(reflect::Value::Field(reflect::Value::Elem(n), 1), 7);
  auto _eo_call_47 = reflect::Value::Type(n);
  auto _eo_call_48 = reflect::Value::Elem(n);
  auto _eo_call_49 = reflect::Value::Interface(_eo_call_48);
  auto _eo_call_50 = reflect::Value::Interface(n);
  fmt::Println(_eo_call_47, _eo_call_49, eo::type_assert<ptr<Point>>(_eo_call_50)->Y);
  auto z = reflect::Zero(reflect::TypeOf(Point{}));
  auto _eo_call_51 = reflect::Value::IsZero(z);
  auto _eo_call_52 = reflect::Value::CanSet(z);
  fmt::Println(z, _eo_call_51, _eo_call_52);

  auto negZero = double{0.0};
  negZero = -negZero;
  auto _eo_call_53 = reflect::ValueOf(double{0.0});
  auto _eo_call_54 = reflect::Value::IsZero(_eo_call_53);
  auto _eo_call_55 = reflect::ValueOf(negZero);
  auto _eo_call_56 = reflect::Value::IsZero(_eo_call_55);
  auto _eo_call_57 = reflect::ValueOf(std::array<int64_t, 2>{0, 1});
  auto _eo_call_58 = reflect::Value::IsZero(_eo_call_57);
  auto _eo_call_59 = reflect::ValueOf(Point{});
  auto _eo_call_60 = reflect::Value::IsZero(_eo_call_59);
  auto _eo_call_61 = reflect::ValueOf(""s);
  auto _eo_call_62 = reflect::Value::IsZero(_eo_call_61);
  auto _eo_call_63 = reflect::ValueOf(ptr<int64_t>(nullptr));
  auto _eo_call_64 = reflect::Value::IsZero(_eo_call_63);
  auto _eo_call_65 = reflect::ValueOf(Tagged{});
  auto _eo_call_66 = reflect::Value::IsZero(_eo_call_65);
  auto _eo_call_67 = reflect::ValueOf(Tagged{.Raw = std::array<uint8_t, 4>{0, 0, 1, 0}});
  auto _eo_call_68 = reflect::Value::IsZero(_eo_call_67);
  fmt::Println(_eo_call_54, _eo_call_56, _eo_call_58, _eo_call_60, _eo_call_62, _eo_call_64, _eo_call_66, _eo_call_68);

  ptr<Point> np;
  slice<int64_t> ns;
  map<std::string, int64_t> nm;
  error ne;
  auto _eo_call_69 = reflect::ValueOf(np);
  auto _eo_call_70 = reflect::Value::IsNil(_eo_call_69);
  auto _eo_call_71 = reflect::ValueOf(ns);
  auto _eo_call_72 = reflect::Value::IsNil(_eo_call_71);
  auto _eo_call_73 = reflect::ValueOf(nm);
  auto _eo_call_74 = reflect::Value::IsNil(_eo_call_73);
  auto _eo_call_75 = reflect::ValueOf(ref(ne));
  auto _eo_call_76 = reflect::Value::Elem(_eo_call_75);
  auto _eo_call_77 = reflect::Value::IsNil(_eo_call_76);
  auto _eo_call_78 = reflect::ValueOf(ref(ne));
  auto _eo_call_79 = reflect::Value::Elem(_eo_call_78);
  auto _eo_call_80 = reflect::Value::Kind(_eo_call_79);
  auto _eo_call_81 = reflect::ValueOf(ref(ne));
  auto _eo_call_82 = reflect::Value::Elem(_eo_call_81);
  auto _eo_call_83 = reflect::Value::Elem(_eo_call_82);
  auto _eo_call_84 = reflect::Value::IsValid(_eo_call_83);
  fmt::Println(_eo_call_70, _eo_call_72, _eo_call_74, _eo_call_77, _eo_call_80, _eo_call_84);

  auto _eo_call_85 = reflect::ValueOf(int8_t{0});
  auto _eo_call_86 = reflect::Value::OverflowInt(_eo_call_85, 200);
  auto _eo_call_87 = reflect::ValueOf(int8_t{0});
  auto _eo_call_88 = reflect::Value::OverflowInt(_eo_call_87, -128);
  auto _eo_call_89 = reflect::ValueOf(uint16_t{0u});
  auto _eo_call_90 = reflect::Value::OverflowUint(_eo_call_89, 70000);
  auto _eo_call_91 = reflect::ValueOf(float{0.0f});
  auto _eo_call_92 = reflect::Value::OverflowFloat(_eo_call_91, 1e39);
  auto _eo_call_93 = reflect::ValueOf(double{0.0});
  auto _eo_call_94 = reflect::Value::OverflowFloat(_eo_call_93, 1e300);
  auto _eo_call_95 = reflect::ValueOf(int64_t{0});
  auto _eo_call_96 = reflect::Value::OverflowInt(_eo_call_95, int64_t{-4611686018427387904});
  fmt::Println(_eo_call_86, _eo_call_88, _eo_call_90, _eo_call_92, _eo_call_94, _eo_call_96);

  auto [x, ok] = reflect::TypeAssert<int64_t>(reflect::ValueOf(int64_t{5}));
  fmt::Println(x, ok);
  std::string s{};
  std::tie(s, ok) = reflect::TypeAssert<std::string>(reflect::ValueOf(int64_t{5}));
  fmt::Printf("%q %v\n"s, s, ok);
  fmt::Stringer st{};
  std::tie(st, ok) = reflect::TypeAssert<fmt::Stringer>(reflect::ValueOf(Celsius{3.0}));
  fmt::Println(st, ok);
  ok = std::get<1>(reflect::TypeAssert<fmt::Stringer>(reflect::ValueOf(Point{})));
  fmt::Println(ok);
}

void setters() {
  auto t = New<Tagged>();
  auto v = reflect::Value::Elem(reflect::ValueOf(t));
  reflect::Value::SetString(reflect::Value::FieldByIndex(v, slice<int64_t>::of({0})), "n"s);
  reflect::Value::SetInt(reflect::Value::Field(v, 1), 3);
  reflect::Value::SetInt(reflect::Value::Field(reflect::Value::Field(v, 3), 0), 42);
  auto _eo_call_0 = reflect::Value::Field(v, 5);
  auto _eo_call_1 = reflect::ValueOf(slice<std::string>::of({"a"s, "b"s}));
  reflect::Value::Set(_eo_call_0, _eo_call_1);
  auto _eo_call_2 = reflect::Value::Field(v, 6);
  auto _eo_call_3 = reflect::Value::Field(v, 6);
  auto _eo_call_4 = reflect::Value::Type(_eo_call_3);
  auto _eo_call_5 = reflect::MakeMap(_eo_call_4);
  reflect::Value::Set(_eo_call_2, _eo_call_5);
  auto _eo_call_6 = reflect::Value::Field(v, 6);
  auto _eo_call_7 = reflect::ValueOf("k"s);
  auto _eo_call_8 = reflect::ValueOf(int64_t{1});
  reflect::Value::SetMapIndex(_eo_call_6, _eo_call_7, _eo_call_8);
  auto _eo_call_9 = reflect::Value::Field(v, 7);
  auto _eo_call_10 = errors::New("boom"s);
  auto _eo_call_11 = reflect::ValueOf(_eo_call_10);
  reflect::Value::Set(_eo_call_9, _eo_call_11);
  auto _eo_call_12 = reflect::Value::Field(v, 8);
  auto _eo_call_13 = reflect::ValueOf(Point{1, 1});
  reflect::Value::Set(_eo_call_12, _eo_call_13);
  reflect::Value::SetFloat(reflect::Value::Field(v, 10), 0.5);
  auto _eo_call_14 = reflect::Value::Field(v, 4);
  auto _eo_call_15 = reflect::Value::Field(v, 4);
  auto _eo_call_16 = reflect::Value::Type(_eo_call_15);
  auto _eo_call_17 = _eo_call_16.Elem();
  auto _eo_call_18 = reflect::New(_eo_call_17);
  reflect::Value::Set(_eo_call_14, _eo_call_18);
  reflect::Value::SetString(reflect::Value::Field(reflect::Value::Elem(reflect::Value::Field(v, 4)), 0), "child"s);
  fmt::Println(t->Name,
    t->Count,
    t->Base.ID,
    t->Tags,
    t->Attrs,
    t->Err,
    t->Any,
    t->F,
    t->Next->Name,
    ::Base::Describe(t->Base));
  auto _eo_call_19 = reflect::Value::Field(v, 7);
  auto _eo_call_20 = reflect::Value::Kind(_eo_call_19);
  auto _eo_call_21 = reflect::Value::Field(v, 7);
  auto _eo_call_22 = reflect::Value::Elem(_eo_call_21);
  auto _eo_call_23 = reflect::Value::Type(_eo_call_22);
  auto _eo_call_24 = reflect::Value::Field(v, 8);
  auto _eo_call_25 = reflect::Value::Elem(_eo_call_24);
  auto _eo_call_26 = reflect::Value::Type(_eo_call_25);
  auto _eo_call_27 = reflect::Value::Field(v, 8);
  auto _eo_call_28 = reflect::Value::Elem(_eo_call_27);
  auto _eo_call_29 = reflect::Value::Field(_eo_call_28, 0);
  auto _eo_call_30 = reflect::Value::Field(v, 8);
  auto _eo_call_31 = reflect::Value::NumMethod(_eo_call_30);
  auto _eo_call_32 = reflect::Value::Field(v, 7);
  auto _eo_call_33 = reflect::Value::NumMethod(_eo_call_32);
  fmt::Println(_eo_call_20, _eo_call_23, _eo_call_26, _eo_call_29, _eo_call_31, _eo_call_33);
  auto _eo_call_34 = reflect::Value::Field(v, 8);
  auto _eo_call_35 = reflect::Value::Field(v, 8);
  auto _eo_call_36 = reflect::Value::Type(_eo_call_35);
  auto _eo_call_37 = reflect::Zero(_eo_call_36);
  reflect::Value::Set(_eo_call_34, _eo_call_37);
  fmt::Println(t->Any == nullptr);
  auto _eo_call_38 = reflect::Value::Field(v, 8);
  auto _eo_call_39 = reflect::ValueOf(ptr(t, &t->Count));
  reflect::Value::Set(_eo_call_38, _eo_call_39);
  *eo::type_assert<ptr<int64_t>>(t->Any) = 4;
  fmt::Println(t->Count);
  auto _eo_call_40 = reflect::Value::Field(v, 2);
  auto _eo_call_41 = reflect::Value::CanSet(_eo_call_40);
  auto _eo_call_42 = reflect::Value::Field(v, 2);
  auto _eo_call_43 = reflect::Value::CanInterface(_eo_call_42);
  auto _eo_call_44 = reflect::Value::Field(v, 2);
  auto _eo_call_45 = reflect::Value::Bool(_eo_call_44);
  auto _eo_call_46 = reflect::Value::Field(v, 2);
  auto _eo_call_47 = reflect::Value::CanAddr(_eo_call_46);
  fmt::Println(_eo_call_41, _eo_call_43, _eo_call_45, _eo_call_47);
  reflect::Value::SetZero(reflect::Value::Field(v, 0));
  fmt::Printf("%q\n"s, t->Name);

  auto r = reflect::Value::Bytes(reflect::Value::Field(v, 9));
  eo::at(r, 1) = 9;
  fmt::Println(t->Raw);
  auto _eo_call_48 = reflect::Value::Field(v, 9);
  auto _eo_call_49 = reflect::ValueOf("abcdef"s);
  auto _eo_call_50 = reflect::Copy(_eo_call_48, _eo_call_49);
  fmt::Println(_eo_call_50, t->Raw);

  auto b = New<Bytes>();
  auto bv = reflect::Value::Elem(reflect::ValueOf(b));
  reflect::Value::Set(bv, reflect::ValueOf(eo::bytes_from_string("xy"s)));
  auto _eo_call_51 = reflect::ValueOf(*b);
  auto _eo_call_52 = reflect::Value::Bytes(_eo_call_51);
  auto _eo_call_53 = reflect::Value::Type(bv);
  auto _eo_call_54 = reflect::Value::Kind(bv);
  fmt::Println(*b, _eo_call_52, _eo_call_53, _eo_call_54);
  reflect::Value::SetBytes(bv, eo::bytes_from_string("z"s));
  fmt::Println(eo::string_from_bytes(*b));
  auto raw = New<slice<uint8_t>>();
  auto _eo_call_55 = reflect::ValueOf(raw);
  auto _eo_call_56 = reflect::Value::Elem(_eo_call_55);
  auto _eo_call_57 = reflect::ValueOf(Bytes(eo::bytes_from_string("w"s)));
  reflect::Value::Set(_eo_call_56, _eo_call_57);
  fmt::Println(eo::string_from_bytes(*raw));
  Celsius c{};
  reflect::Value::SetFloat(reflect::Value::Elem(reflect::ValueOf(ref(c))), 36.6);
  fmt::Println(c);
  uint16_t u{};
  reflect::Value::SetUint(reflect::Value::Elem(reflect::ValueOf(ref(u))), 70000);
  fmt::Println(u);
}

void slicesAndArrays() {
  auto s = reflect::MakeSlice(reflect::TypeOf(slice<int64_t>::of({})), 2, 5);
  auto _eo_call_0 = reflect::Value::Len(s);
  auto _eo_call_1 = reflect::Value::Cap(s);
  auto _eo_call_2 = reflect::Value::Interface(s);
  auto _eo_call_3 = reflect::Value::IsNil(s);
  fmt::Println(_eo_call_0, _eo_call_1, _eo_call_2, _eo_call_3);
  reflect::Value::SetInt(reflect::Value::Index(s, 1), 7);
  auto _eo_call_4 = reflect::Value::Interface(s);
  auto _eo_call_5 = reflect::Value::Index(s, 1);
  auto _eo_call_6 = reflect::Value::CanSet(_eo_call_5);
  fmt::Println(_eo_call_4, _eo_call_6);
  auto sp = reflect::New(reflect::Value::Type(s));
  reflect::Value::Set(reflect::Value::Elem(sp), s);
  auto e = reflect::Value::Elem(sp);
  reflect::Value::SetLen(e, 4);
  auto _eo_call_7 = reflect::Value::Len(e);
  auto _eo_call_8 = reflect::Value::Interface(e);
  fmt::Println(_eo_call_7, _eo_call_8);
  reflect::Value::Grow(e, 10);
  auto _eo_call_9 = reflect::Value::Len(e);
  auto _eo_call_10 = reflect::Value::Cap(e);
  auto _eo_call_11 = reflect::Value::Interface(e);
  fmt::Println(_eo_call_9, _eo_call_10 >= 14, _eo_call_11);
  reflect::Value::SetLen(e, reflect::Value::Len(e) + 1);
  reflect::Value::SetInt(reflect::Value::Index(e, 4), 5);
  auto _eo_call_12 = reflect::Value::Interface(e);
  auto _eo_call_13 = reflect::Value::Interface(s);
  fmt::Println(_eo_call_12, _eo_call_13);

  auto arr = std::array<std::string, 3>{"a"s, "b"s, "c"s};
  auto av = reflect::Value::Elem(reflect::ValueOf(ref(arr)));
  reflect::Value::SetString(reflect::Value::Index(av, 0), "A"s);
  auto _eo_call_14 = reflect::Value::Len(av);
  auto _eo_call_15 = reflect::Value::Cap(av);
  auto _eo_call_16 = reflect::ValueOf(arr);
  auto _eo_call_17 = reflect::Value::Index(_eo_call_16, 2);
  auto _eo_call_18 = reflect::ValueOf(arr);
  auto _eo_call_19 = reflect::Value::Index(_eo_call_18, 2);
  auto _eo_call_20 = reflect::Value::CanSet(_eo_call_19);
  fmt::Println(arr, _eo_call_14, _eo_call_15, _eo_call_17, _eo_call_20);
  auto _eo_call_21 = reflect::ValueOf("héllo"s);
  auto _eo_call_22 = reflect::Value::Len(_eo_call_21);
  auto _eo_call_23 = reflect::ValueOf("hey"s);
  auto _eo_call_24 = reflect::Value::Index(_eo_call_23, 1);
  auto _eo_call_25 = reflect::Value::Uint(_eo_call_24);
  auto _eo_call_26 = reflect::ValueOf(ref(arr));
  auto _eo_call_27 = reflect::Value::Len(_eo_call_26);
  fmt::Println(_eo_call_22, _eo_call_25, _eo_call_27);

  auto ints = slice<int64_t>::of({1, 2, 3, 4, 5});
  auto _eo_call_28 = reflect::ValueOf(eo::subslice(ints, 1));
  auto _eo_call_29 = reflect::ValueOf(ints);
  auto n = reflect::Copy(_eo_call_28, _eo_call_29);
  fmt::Println(n, ints);
  ints = slice<int64_t>::of({1, 2, 3, 4, 5});
  auto _eo_call_30 = reflect::ValueOf(ints);
  auto _eo_call_31 = reflect::ValueOf(eo::subslice(ints, 2));
  n = reflect::Copy(_eo_call_30, _eo_call_31);
  fmt::Println(n, ints);
  ints = slice<int64_t>::of({1, 2, 3, 4, 5});
  auto _eo_call_32 = reflect::ValueOf(eo::subslice(ints, 2, 5));
  auto _eo_call_33 = reflect::ValueOf(eo::subslice(ints, 0, 3));
  n = reflect::Copy(_eo_call_32, _eo_call_33);
  fmt::Println(n, ints);
  auto dst = eo::make_slice<uint8_t>(3);
  auto _eo_call_34 = reflect::ValueOf(dst);
  auto _eo_call_35 = reflect::ValueOf("xyzw"s);
  auto _eo_call_36 = reflect::Copy(_eo_call_34, _eo_call_35);
  fmt::Println(_eo_call_36, eo::string_from_bytes(dst));
  auto _eo_call_37 = reflect::TypeOf(Bytes{slice<uint8_t>::of({})});
  auto _eo_call_38 = reflect::MakeSlice(_eo_call_37, 0, 0);
  auto _eo_call_39 = reflect::Value::Interface(_eo_call_38);
  auto _eo_call_40 = reflect::TypeOf(Bytes{slice<uint8_t>::of({})});
  auto _eo_call_41 = reflect::MakeSlice(_eo_call_40, 0, 0);
  auto _eo_call_42 = reflect::Value::IsNil(_eo_call_41);
  fmt::Println(_eo_call_39, _eo_call_42);
}

void embedded() {
  auto w = New<Wrapper>(Wrapper{.inner = inner{.Val = 1}});
  auto v = reflect::Value::Elem(reflect::ValueOf(w));
  auto in = reflect::Value::Field(v, 0);
  auto _eo_call_0 = reflect::Value::CanSet(in);
  auto _eo_call_1 = reflect::Value::CanInterface(in);
  auto _eo_call_2 = reflect::Value::Field(in, 0);
  auto _eo_call_3 = reflect::Value::CanSet(_eo_call_2);
  auto _eo_call_4 = reflect::Value::Field(in, 0);
  auto _eo_call_5 = reflect::Value::CanInterface(_eo_call_4);
  auto _eo_call_6 = reflect::Value::Field(in, 1);
  auto _eo_call_7 = reflect::Value::CanSet(_eo_call_6);
  fmt::Println(_eo_call_0, _eo_call_1, _eo_call_3, _eo_call_5, _eo_call_7);
  reflect::Value::SetInt(reflect::Value::Field(in, 0), 9);
  auto _eo_call_8 = reflect::Value::Field(in, 0);
  auto _eo_call_9 = reflect::Value::Interface(_eo_call_8);
  fmt::Println(w->inner.Val, _eo_call_9);
  auto p = reflect::Value::Addr(in);
  auto _eo_call_10 = reflect::Value::CanInterface(p);
  auto _eo_call_11 = reflect::Value::Elem(p);
  auto _eo_call_12 = reflect::Value::CanSet(_eo_call_11);
  auto _eo_call_13 = reflect::Value::Elem(p);
  auto _eo_call_14 = reflect::Value::Field(_eo_call_13, 0);
  auto _eo_call_15 = reflect::Value::CanSet(_eo_call_14);
  auto _eo_call_16 = reflect::Value::Elem(p);
  auto _eo_call_17 = reflect::Value::Field(_eo_call_16, 1);
  auto _eo_call_18 = reflect::Value::CanSet(_eo_call_17);
  fmt::Println(_eo_call_10, _eo_call_12, _eo_call_15, _eo_call_18);
  auto ch = reflect::Value::Field(v, 1);
  auto _eo_call_19 = reflect::Value::Kind(ch);
  auto _eo_call_20 = reflect::Value::Type(ch);
  auto _eo_call_21 = reflect::Value::IsNil(ch);
  auto _eo_call_22 = reflect::Value::Type(ch).Elem();
  auto _eo_call_23 = reflect::Value::CanSet(ch);
  fmt::Println(_eo_call_19, _eo_call_20, _eo_call_21, _eo_call_22, _eo_call_23);
}

void maps() {
  auto m = map<std::string, int64_t>::of({{"a"s, 1}, {"b"s, 2}});
  auto mv = reflect::ValueOf(m);
  auto _eo_call_0 = reflect::Value::Len(mv);
  auto _eo_call_1 = reflect::ValueOf("a"s);
  auto _eo_call_2 = reflect::Value::MapIndex(mv, _eo_call_1);
  auto _eo_call_3 = reflect::ValueOf("z"s);
  auto _eo_call_4 = reflect::Value::MapIndex(mv, _eo_call_3);
  auto _eo_call_5 = reflect::Value::IsValid(_eo_call_4);
  fmt::Println(_eo_call_0, _eo_call_2, _eo_call_5);
  slice<std::string> names;
  for (auto [_eo_range_key, k] : eo::range(reflect::Value::MapKeys(mv))) {
    auto _eo_call_6 = reflect::Value::String(k);
    names = eo::append(names, {_eo_call_6});
  }
  slices::SortFunc(names, [=](std::string a, std::string b) mutable -> int64_t {
    if (a < b) {
      return -1;
    }
    if (a > b) {
      return 1;
    }
    return 0;
  });
  fmt::Println(names);
  auto _eo_call_7 = reflect::ValueOf("c"s);
  auto _eo_call_8 = reflect::ValueOf(int64_t{3});
  reflect::Value::SetMapIndex(mv, _eo_call_7, _eo_call_8);
  reflect::Value::SetMapIndex(mv, reflect::ValueOf("a"s), reflect::Value{});
  fmt::Println(m);
  auto it = reflect::Value::MapRange(mv);
  auto sum = int64_t{0};
  auto count = int64_t{0};
  while (reflect::MapIter::Next(it)) {
    sum += int64_t(reflect::Value::Int(reflect::MapIter::Value(it)));
    auto _eo_call_9 = reflect::MapIter::Key(it);
    auto _eo_call_10 = reflect::Value::String(_eo_call_9);
    count += eo::len(_eo_call_10);
  }
  fmt::Println(count, sum);
  auto mm = reflect::MakeMapWithSize(reflect::TypeOf(map<int64_t, std::string>::of({})), 4);
  auto _eo_call_11 = reflect::ValueOf(int64_t{1});
  auto _eo_call_12 = reflect::ValueOf("one"s);
  reflect::Value::SetMapIndex(mm, _eo_call_11, _eo_call_12);
  auto _eo_call_13 = reflect::Value::Interface(mm);
  auto _eo_call_14 = reflect::Value::Len(mm);
  auto _eo_call_15 = reflect::ValueOf(map<int64_t, int64_t>::of({}));
  auto _eo_call_16 = reflect::Value::MapKeys(_eo_call_15);
  fmt::Println(_eo_call_13, _eo_call_14, eo::len(_eo_call_16));
  map<std::string, int64_t> nilMap;
  auto _eo_call_17 = reflect::ValueOf(nilMap);
  auto _eo_call_18 = reflect::ValueOf("x"s);
  auto _eo_call_19 = reflect::Value::MapIndex(_eo_call_17, _eo_call_18);
  auto _eo_call_20 = reflect::Value::IsValid(_eo_call_19);
  auto _eo_call_21 = reflect::ValueOf(nilMap);
  auto _eo_call_22 = reflect::Value::Len(_eo_call_21);
  fmt::Println(_eo_call_20, _eo_call_22);
  auto _eo_call_23 = reflect::ValueOf(map<bool, int64_t>::of({{false, 7}}));
  auto _eo_call_24 = reflect::ValueOf(Tagged{});
  auto _eo_call_25 = reflect::Value::Field(_eo_call_24, 2);
  auto found = reflect::Value::MapIndex(_eo_call_23, _eo_call_25);
  auto _eo_call_26 = reflect::Value::Int(found);
  auto _eo_call_27 = reflect::Value::CanInterface(found);
  fmt::Println(_eo_call_26, _eo_call_27);
}

void comparisons() {
  auto t8 = reflect::TypeFor<int8_t>();
  auto _eo_call_0 = t8.OverflowInt(127);
  auto _eo_call_1 = t8.OverflowInt(128);
  auto _eo_call_2 = t8.OverflowInt(-129);
  auto _eo_call_3 = reflect::TypeFor<uint32_t>();
  auto _eo_call_4 = _eo_call_3.OverflowUint(uint64_t{4294967296u});
  auto _eo_call_5 = reflect::TypeFor<float>();
  auto _eo_call_6 = _eo_call_5.OverflowFloat(1e39);
  auto _eo_call_7 = reflect::TypeFor<double>();
  auto _eo_call_8 = _eo_call_7.OverflowFloat(1e300);
  fmt::Println(_eo_call_0, _eo_call_1, _eo_call_2, _eo_call_4, _eo_call_6, _eo_call_8);

  auto p = New<Point>(Point{1, 2});
  auto q = New<Point>(Point{1, 2});
  auto a = New<any>(p);
  auto _eo_call_9 = reflect::ValueOf(p);
  auto _eo_call_10 = reflect::ValueOf(p);
  auto _eo_call_11 = reflect::Value::Equal(_eo_call_9, _eo_call_10);
  auto _eo_call_12 = reflect::ValueOf(p);
  auto _eo_call_13 = reflect::ValueOf(q);
  auto _eo_call_14 = reflect::Value::Equal(_eo_call_12, _eo_call_13);
  auto _eo_call_15 = reflect::ValueOf(*p);
  auto _eo_call_16 = reflect::ValueOf(*q);
  auto _eo_call_17 = reflect::Value::Equal(_eo_call_15, _eo_call_16);
  auto _eo_call_18 = reflect::ValueOf(a);
  auto _eo_call_19 = reflect::Value::Elem(_eo_call_18);
  auto _eo_call_20 = reflect::ValueOf(p);
  auto _eo_call_21 = reflect::Value::Equal(_eo_call_19, _eo_call_20);
  fmt::Println(_eo_call_11, _eo_call_14, _eo_call_17, _eo_call_21);
  auto _eo_call_22 = reflect::ValueOf(std::array<int64_t, 2>{1, 2});
  auto _eo_call_23 = reflect::ValueOf(std::array<int64_t, 2>{1, 2});
  auto _eo_call_24 = reflect::Value::Equal(_eo_call_22, _eo_call_23);
  auto _eo_call_25 = reflect::ValueOf(int64_t{1});
  auto _eo_call_26 = reflect::ValueOf("1"s);
  auto _eo_call_27 = reflect::Value::Equal(_eo_call_25, _eo_call_26);
  auto _eo_call_28 = reflect::Value::Equal(reflect::Value{}, reflect::Value{});
  auto _eo_call_29 = reflect::ValueOf(int64_t{1});
  auto _eo_call_30 = reflect::Value::Equal(_eo_call_29, reflect::Value{});
  fmt::Println(_eo_call_24, _eo_call_27, _eo_call_28, _eo_call_30);

  auto s = slice<int64_t>::of({1, 2, 3});
  auto m = map<std::string, int64_t>::of({{"a"s, 1}});
  auto m2 = m;
  auto _eo_call_31 = reflect::ValueOf(s);
  auto _eo_call_32 = reflect::Value::Pointer(_eo_call_31);
  auto _eo_call_33 = reflect::ValueOf(eo::subslice(s, 0, 2));
  auto _eo_call_34 = reflect::Value::Pointer(_eo_call_33);
  auto _eo_call_35 = reflect::ValueOf(s);
  auto _eo_call_36 = reflect::Value::Pointer(_eo_call_35);
  auto _eo_call_37 = reflect::ValueOf(eo::subslice(s, 1));
  auto _eo_call_38 = reflect::Value::Pointer(_eo_call_37);
  fmt::Println(_eo_call_32 == _eo_call_34, _eo_call_36 == _eo_call_38);
  auto _eo_call_39 = reflect::ValueOf(m);
  auto _eo_call_40 = reflect::Value::Pointer(_eo_call_39);
  auto _eo_call_41 = reflect::ValueOf(m2);
  auto _eo_call_42 = reflect::Value::Pointer(_eo_call_41);
  auto _eo_call_43 = reflect::ValueOf(m);
  auto _eo_call_44 = reflect::Value::Pointer(_eo_call_43);
  auto _eo_call_45 = reflect::ValueOf(map<std::string, int64_t>::of({}));
  auto _eo_call_46 = reflect::Value::Pointer(_eo_call_45);
  auto _eo_call_47 = reflect::ValueOf(p);
  auto _eo_call_48 = reflect::Value::Pointer(_eo_call_47);
  auto _eo_call_49 = reflect::ValueOf(q);
  auto _eo_call_50 = reflect::Value::Pointer(_eo_call_49);
  auto _eo_call_51 = reflect::ValueOf(ptr<int64_t>{});
  auto _eo_call_52 = reflect::Value::Pointer(_eo_call_51);
  fmt::Println(_eo_call_40 == _eo_call_42, _eo_call_44 == _eo_call_46, _eo_call_48 == _eo_call_50, _eo_call_52);
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

void panics() {
  attempt("Int of string"s, [=]() mutable { reflect::Value::Int(reflect::ValueOf("s"s)); });
  attempt("Type of zero"s, [=]() mutable { reflect::Value::Type(reflect::Value{}); });
  attempt("SetInt unaddressable"s, [=]() mutable { reflect::Value::SetInt(reflect::ValueOf(int64_t{1}), 2); });
  attempt("SetBool unexported"s, [=]() mutable {
    reflect::Value::SetBool(reflect::Value::Field(reflect::Value::Elem(reflect::ValueOf(New<Tagged>(Tagged{}))), 2),
      true);
  });
  attempt("Interface unexported"s,
    [=]() mutable { reflect::Value::Interface(reflect::Value::Field(reflect::ValueOf(Tagged{}), 2)); });
  attempt("Set not assignable"s, [=]() mutable {
    auto x = New<int64_t>();
    auto _eo_call_0 = reflect::ValueOf(x);
    auto _eo_call_1 = reflect::Value::Elem(_eo_call_0);
    auto _eo_call_2 = reflect::ValueOf("s"s);
    reflect::Value::Set(_eo_call_1, _eo_call_2);
  });
  attempt("Set zero Value"s, [=]() mutable {
    auto x = New<int64_t>();
    reflect::Value::Set(reflect::Value::Elem(reflect::ValueOf(x)), reflect::Value{});
  });
  attempt("Elem of int type"s, [=]() mutable { reflect::TypeOf(int64_t{1}).Elem(); });
  attempt("Elem of int"s, [=]() mutable { reflect::Value::Elem(reflect::ValueOf(int64_t{1})); });
  attempt("Field out of range"s, [=]() mutable { reflect::Value::Field(reflect::ValueOf(Point{}), 5); });
  attempt("Type.Field out of range"s, [=]() mutable { reflect::TypeOf(Point{}).Field(5); });
  attempt("Index out of range"s,
    [=]() mutable { reflect::Value::Index(reflect::ValueOf(slice<int64_t>::of({1})), 3); });
  attempt("SetLen out of range"s, [=]() mutable {
    auto s = slice<int64_t>::of({1});
    reflect::Value::SetLen(reflect::Value::Elem(reflect::ValueOf(ref(s))), 5);
  });
  attempt("Bytes of ints"s, [=]() mutable { reflect::Value::Bytes(reflect::ValueOf(slice<int64_t>::of({1}))); });
  attempt("Bytes of unaddressable array"s,
    [=]() mutable { reflect::Value::Bytes(reflect::ValueOf(std::array<uint8_t, 2>{})); });
  attempt("New nil"s, [=]() mutable { reflect::New(nullptr); });
  attempt("MakeSlice of map"s,
    [=]() mutable { reflect::MakeSlice(reflect::TypeOf(map<int64_t, int64_t>::of({})), 0, 0); });
  attempt("MakeSlice len > cap"s, [=]() mutable { reflect::MakeSlice(reflect::TypeOf(slice<int64_t>::of({})), 2, 1); });
  attempt("Implements non-interface"s, [=]() mutable {
    auto _eo_call_3 = reflect::TypeOf(int64_t{1});
    auto _eo_call_4 = reflect::TypeOf(int64_t{1});
    _eo_call_3.Implements(_eo_call_4);
  });
  attempt("NumField of int"s, [=]() mutable { reflect::TypeOf(int64_t{1}).NumField(); });
  attempt("Key of slice"s, [=]() mutable { reflect::TypeOf(slice<int64_t>::of({})).Key(); });
  attempt("Len of slice type"s, [=]() mutable { reflect::TypeOf(slice<int64_t>::of({})).Len(); });
  attempt("Bits of string"s, [=]() mutable { reflect::TypeOf(""s).Bits(); });
  attempt("MapIter exhausted"s, [=]() mutable {
    auto it = reflect::Value::MapRange(reflect::ValueOf(map<int64_t, int64_t>::of({})));
    reflect::MapIter::Next(it);
    reflect::MapIter::Next(it);
  });
  attempt("MapIter Key before Next"s, [=]() mutable {
    reflect::MapIter::Key(reflect::Value::MapRange(reflect::ValueOf(map<int64_t, int64_t>::of({}))));
  });
  attempt("SetMapIndex wrong key"s, [=]() mutable {
    auto _eo_call_5 = reflect::ValueOf(map<int64_t, int64_t>::of({}));
    auto _eo_call_6 = reflect::ValueOf("k"s);
    auto _eo_call_7 = reflect::ValueOf(int64_t{1});
    reflect::Value::SetMapIndex(_eo_call_5, _eo_call_6, _eo_call_7);
  });
  attempt("SetMapIndex nil map"s, [=]() mutable {
    map<int64_t, int64_t> m;
    auto _eo_call_8 = reflect::ValueOf(m);
    auto _eo_call_9 = reflect::ValueOf(int64_t{1});
    auto _eo_call_10 = reflect::ValueOf(int64_t{1});
    reflect::Value::SetMapIndex(_eo_call_8, _eo_call_9, _eo_call_10);
  });
  attempt("Copy mismatch"s, [=]() mutable {
    auto _eo_call_11 = reflect::ValueOf(slice<int64_t>::of({1}));
    auto _eo_call_12 = reflect::ValueOf(slice<std::string>::of({"a"s}));
    reflect::Copy(_eo_call_11, _eo_call_12);
  });
  attempt("Copy into int"s, [=]() mutable {
    auto _eo_call_13 = reflect::ValueOf(int64_t{1});
    auto _eo_call_14 = reflect::ValueOf(slice<int64_t>::of({1}));
    reflect::Copy(_eo_call_13, _eo_call_14);
  });
  attempt("IsNil of int"s, [=]() mutable { reflect::Value::IsNil(reflect::ValueOf(int64_t{1})); });
  attempt("IsZero of zero"s, [=]() mutable { reflect::Value::IsZero(reflect::Value{}); });
  attempt("Addr unaddressable"s, [=]() mutable { reflect::Value::Addr(reflect::ValueOf(int64_t{1})); });
  attempt("Len of int"s, [=]() mutable { reflect::Value::Len(reflect::ValueOf(int64_t{1})); });
  attempt("Grow negative"s, [=]() mutable {
    auto s = slice<int64_t>::of({});
    reflect::Value::Grow(reflect::Value::Elem(reflect::ValueOf(ref(s))), -1);
  });
  attempt("Float of int"s, [=]() mutable { reflect::Value::Float(reflect::ValueOf(int64_t{1})); });
  attempt("SetString of int"s, [=]() mutable {
    int64_t x{};
    reflect::Value::SetString(reflect::Value::Elem(reflect::ValueOf(ref(x))), "s"s);
  });
  attempt("TypeAssert zero"s, [=]() mutable { reflect::TypeAssert<int64_t>(reflect::Value{}); });
  attempt("FieldByIndex nil embedded"s, [=]() mutable {
    auto _eo_call_15 = reflect::ValueOf(Outer{});
    reflect::Value::FieldByIndex(_eo_call_15, slice<int64_t>::of({0, 1}));
  });
  attempt("Equal of slices"s, [=]() mutable {
    auto _eo_call_16 = reflect::ValueOf(slice<int64_t>::of({}));
    auto _eo_call_17 = reflect::ValueOf(slice<int64_t>::of({}));
    reflect::Value::Equal(_eo_call_16, _eo_call_17);
  });
  attempt("Pointer of int"s, [=]() mutable { reflect::Value::Pointer(reflect::ValueOf(int64_t{1})); });
  attempt("OverflowInt of string"s, [=]() mutable { reflect::TypeFor<std::string>().OverflowInt(1); });
  attempt("OverflowUint of int"s, [=]() mutable { reflect::TypeFor<int64_t>().OverflowUint(1); });
  attempt("OverflowFloat of int"s, [=]() mutable { reflect::TypeFor<int64_t>().OverflowFloat(1); });
}

func<> eo_main() {
  kinds();
  types();
  structFields();
  values();
  setters();
  slicesAndArrays();
  embedded();
  maps();
  comparisons();
  panics();
  co_return;
}
