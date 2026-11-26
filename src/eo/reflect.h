// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/fmt.h>
#include <eo/interface.h>
#include <eo/map.h>
#include <eo/panic.h>
#include <eo/ptr.h>
#include <eo/rtype.h>
#include <eo/slice.h>
#include <eo/strconv.h>

#include <algorithm>
#include <array>
#include <cfloat>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

// Go's reflect package, ported from Go 1.26's reflect on the type information of <eo/rtype.h>. A
// Type is the information of one type, and a Value holds the type and the address of the value's
// storage, shared with whatever owns that storage, as a Go reflect.Value points into the heap.

namespace eo::reflect {

// Go's reflect.Kind.
struct Kind : eo::defined<Kind, uint64_t> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "reflect.Kind";

  static auto String(Kind k) -> std::string;

  using _eo_methods = eo::methods<eo::method<&Kind::String, "String">>;
};

inline constexpr Kind Invalid{uint64_t{eo::detail::rk_invalid}};
inline constexpr Kind Bool{uint64_t{eo::detail::rk_bool}};
inline constexpr Kind Int{uint64_t{eo::detail::rk_int}};
inline constexpr Kind Int8{uint64_t{eo::detail::rk_int8}};
inline constexpr Kind Int16{uint64_t{eo::detail::rk_int16}};
inline constexpr Kind Int32{uint64_t{eo::detail::rk_int32}};
inline constexpr Kind Int64{uint64_t{eo::detail::rk_int64}};
inline constexpr Kind Uint{uint64_t{eo::detail::rk_uint}};
inline constexpr Kind Uint8{uint64_t{eo::detail::rk_uint8}};
inline constexpr Kind Uint16{uint64_t{eo::detail::rk_uint16}};
inline constexpr Kind Uint32{uint64_t{eo::detail::rk_uint32}};
inline constexpr Kind Uint64{uint64_t{eo::detail::rk_uint64}};
inline constexpr Kind Uintptr{uint64_t{eo::detail::rk_uintptr}};
inline constexpr Kind Float32{uint64_t{eo::detail::rk_float32}};
inline constexpr Kind Float64{uint64_t{eo::detail::rk_float64}};
inline constexpr Kind Complex64{uint64_t{eo::detail::rk_complex64}};
inline constexpr Kind Complex128{uint64_t{eo::detail::rk_complex128}};
inline constexpr Kind Array{uint64_t{eo::detail::rk_array}};
inline constexpr Kind Chan{uint64_t{eo::detail::rk_chan}};
inline constexpr Kind Func{uint64_t{eo::detail::rk_func}};
inline constexpr Kind Interface{uint64_t{eo::detail::rk_interface}};
inline constexpr Kind Map{uint64_t{eo::detail::rk_map}};
inline constexpr Kind Pointer{uint64_t{eo::detail::rk_pointer}};
inline constexpr Kind Slice{uint64_t{eo::detail::rk_slice}};
inline constexpr Kind String{uint64_t{eo::detail::rk_string}};
inline constexpr Kind Struct{uint64_t{eo::detail::rk_struct}};
inline constexpr Kind UnsafePointer{uint64_t{eo::detail::rk_unsafe_pointer}};
inline constexpr Kind Ptr = Pointer;

inline auto Kind::String(Kind k) -> std::string {
  static constexpr std::array<std::string_view, 27> names{"invalid",
    "bool",
    "int",
    "int8",
    "int16",
    "int32",
    "int64",
    "uint",
    "uint8",
    "uint16",
    "uint32",
    "uint64",
    "uintptr",
    "float32",
    "float64",
    "complex64",
    "complex128",
    "array",
    "chan",
    "func",
    "interface",
    "map",
    "ptr",
    "slice",
    "string",
    "struct",
    "unsafe.Pointer"};
  if (k._eo_value < names.size()) {
    return std::string{names[k._eo_value]};
  }
  return "kind" + strconv::Itoa(static_cast<int64_t>(k._eo_value));
}

// Go's reflect.ValueError, which a Value method panics with when called on a Value of the wrong kind.
struct ValueError {
  static constexpr std::string_view _eo_name = "reflect.ValueError";

  std::string Method{};
  reflect::Kind Kind{};

  auto operator==(const ValueError&) const -> bool = default;

  static auto Error(ptr<ValueError> e) -> std::string {
    if (e->Kind == Invalid) {
      return "reflect: call of " + e->Method + " on zero Value";
    }
    return "reflect: call of " + e->Method + " on " + reflect::Kind::String(e->Kind) + " Value";
  }

  using _eo_methods = eo::methods<eo::method<&ValueError::Error, "Error">>;
};

// Go's reflect.StructTag.
struct StructTag : eo::defined<StructTag, std::string> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "reflect.StructTag";

  static auto Get(StructTag tag, std::string key) -> std::string {
    auto [v, _] = Lookup(std::move(tag), std::move(key));
    return v;
  }

  static auto Lookup(StructTag tag, std::string key) -> std::tuple<std::string, bool>;

  using _eo_methods = eo::methods<eo::method<&StructTag::Get, "Get">, eo::method<&StructTag::Lookup, "Lookup">>;
};

inline auto StructTag::Lookup(StructTag tag_, std::string key) -> std::tuple<std::string, bool> {
  auto tag = std::string_view{tag_};
  while (!tag.empty()) {
    // Skip leading space.
    auto i = std::size_t{0};
    while (i < tag.size() && tag[i] == ' ') {
      ++i;
    }
    tag = tag.substr(i);
    if (tag.empty()) {
      break;
    }
    // Scan to colon. A space, a quote or a control character is a syntax error.
    i = 0;
    while (i < tag.size() && static_cast<uint8_t>(tag[i]) > ' ' && tag[i] != ':' && tag[i] != '"' &&
      static_cast<uint8_t>(tag[i]) != 0x7f) {
      ++i;
    }
    if (i == 0 || i + 1 >= tag.size() || tag[i] != ':' || tag[i + 1] != '"') {
      break;
    }
    auto name = tag.substr(0, i);
    tag = tag.substr(i + 1);
    // Scan quoted string to find value.
    i = 1;
    while (i < tag.size() && tag[i] != '"') {
      if (tag[i] == '\\') {
        ++i;
      }
      ++i;
    }
    if (i >= tag.size()) {
      break;
    }
    auto qvalue = std::string{tag.substr(0, i + 1)};
    tag = tag.substr(i + 1);
    if (key == name) {
      auto [value, err] = strconv::Unquote(qvalue);
      if (err != nullptr) {
        break;
      }
      return {value, true};
    }
  }
  return {"", false};
}

struct StructField;
struct Value;

// Go's reflect.Type. Go's Type is an interface that only *rtype implements, so Eo maps it to one
// class, which holds the type's information and calls its methods as members, as a translated
// interface does. The zero Type is nil.
struct Type {
  static constexpr std::string_view _eo_name = "*reflect.rtype";

  const eo::detail::rtype* _eo_t = nullptr;

  constexpr Type() noexcept = default;
  constexpr Type(std::nullptr_t) noexcept {}
  constexpr explicit Type(const eo::detail::rtype* t) noexcept: _eo_t(t) {}

  friend auto operator==(Type a, Type b) noexcept -> bool {
    return a._eo_t == b._eo_t;
  }

  friend auto operator==(Type a, std::nullptr_t) noexcept -> bool {
    return a._eo_t == nullptr;
  }

  // A Type's identity, by which it is a map key, as Go's *rtype is a pointer.
  auto _eo_key() const noexcept -> const void* {
    return _eo_t;
  }

  auto Kind() const -> reflect::Kind {
    return reflect::Kind{t()->kind};
  }

  auto Name() const -> std::string {
    return std::string{t()->name};
  }

  auto String() const -> std::string {
    return t()->string();
  }

  auto Elem() const -> Type {
    switch (t()->kind) {
    case eo::detail::rk_array:
    case eo::detail::rk_chan:
    case eo::detail::rk_map:
    case eo::detail::rk_pointer:
    case eo::detail::rk_slice:
      return Type{t()->elem()};
    default:
      eo::panic("reflect: Elem of invalid type " + String());
    }
  }

  auto Key() const -> Type {
    if (t()->kind != eo::detail::rk_map) {
      eo::panic("reflect: Key of non-map type " + String());
    }
    return Type{t()->key()};
  }

  auto Len() const -> int64_t {
    if (t()->kind != eo::detail::rk_array) {
      eo::panic("reflect: Len of non-array type " + String());
    }
    return t()->len;
  }

  auto NumField() const -> int64_t {
    if (t()->kind != eo::detail::rk_struct) {
      eo::panic("reflect: NumField of non-struct type " + String());
    }
    return static_cast<int64_t>(t()->fields().size());
  }

  auto Field(int64_t i) const -> StructField;

  auto NumMethod() const -> int64_t;

  auto Implements(Type u) const -> bool {
    if (u == nullptr) {
      eo::panic("reflect: nil type passed to Type.Implements");
    }
    if (u.Kind() != Interface) {
      eo::panic("reflect: non-interface type passed to Type.Implements");
    }
    return implements(u._eo_t, t());
  }

  auto AssignableTo(Type u) const -> bool {
    if (u == nullptr) {
      eo::panic("reflect: nil type passed to Type.AssignableTo");
    }
    return directly_assignable(u._eo_t, t()) || implements(u._eo_t, t());
  }

  auto Comparable() const -> bool {
    return t()->equal != nullptr;
  }

  auto Bits() const -> int64_t {
    switch (t()->kind) {
    case eo::detail::rk_int8:
    case eo::detail::rk_uint8:
      return 8;
    case eo::detail::rk_int16:
    case eo::detail::rk_uint16:
      return 16;
    case eo::detail::rk_int32:
    case eo::detail::rk_uint32:
    case eo::detail::rk_float32:
      return 32;
    case eo::detail::rk_int:
    case eo::detail::rk_int64:
    case eo::detail::rk_uint:
    case eo::detail::rk_uint64:
    case eo::detail::rk_uintptr:
    case eo::detail::rk_float64:
      return 64;
    default:
      eo::panic("reflect: Bits of non-arithmetic Type " + String());
    }
  }

  auto OverflowFloat(double x) const -> bool {
    switch (t()->kind) {
    case eo::detail::rk_float32: {
      auto a = x < 0 ? -x : x;
      return FLT_MAX < a && a <= DBL_MAX;
    }
    case eo::detail::rk_float64:
      return false;
    default:
      eo::panic("reflect: OverflowFloat of non-float type " + String());
    }
  }

  auto OverflowInt(int64_t x) const -> bool {
    if (t()->kind < eo::detail::rk_int || t()->kind > eo::detail::rk_int64) {
      eo::panic("reflect: OverflowInt of non-int type " + String());
    }
    auto bits = Bits();
    auto trunc = static_cast<int64_t>(static_cast<uint64_t>(x) << (64 - bits)) >> (64 - bits);
    return x != trunc;
  }

  auto OverflowUint(uint64_t x) const -> bool {
    if (t()->kind < eo::detail::rk_uint || t()->kind > eo::detail::rk_uintptr) {
      eo::panic("reflect: OverflowUint of non-uint type " + String());
    }
    auto bits = Bits();
    auto trunc = (x << (64 - bits)) >> (64 - bits);
    return x != trunc;
  }

  // Calling a method of a nil Type panics, as calling a method of a nil interface value does.
  auto t() const -> const eo::detail::rtype* {
    if (_eo_t == nullptr) {
      eo::detail::panic_nil_dereference();
    }
    return _eo_t;
  }

  // Go's implements: whether V has every method of the interface type T.
  static auto implements(const eo::detail::rtype* T, const eo::detail::rtype* V) -> bool {
    if (T->kind != eo::detail::rk_interface) {
      return false;
    }
    auto wanted = T->imethods();
    if (wanted.empty()) {
      return true;
    }
    auto has = [&](const eo::detail::entry_base& m) {
      if (V->kind == eo::detail::rk_interface) {
        return std::ranges::any_of(V->imethods(),
          [&](const auto& v) { return v.name == m.name && v.signature == m.signature; });
      }
      const auto* d = V->desc();
      if (d == nullptr) {
        return false;
      }
      return std::ranges::any_of(d->methods,
        [&](const auto* v) { return v->name == m.name && v->signature == m.signature; });
    };
    return std::ranges::all_of(wanted, has);
  }

  // Go's directlyAssignable: whether a value of type V can be assigned to type T without converting
  // it to an interface: when the types are identical, or have the same underlying type and one of
  // them is not named.
  static auto directly_assignable(const eo::detail::rtype* T, const eo::detail::rtype* V) -> bool {
    if (T == V) {
      return true;
    }
    if (!T->name.empty() && !V->name.empty()) {
      return false;
    }
    return T->kind != eo::detail::rk_interface && T->rep_type() == V->rep_type();
  }

  static auto _eo_String(Type t) -> std::string {
    return t.String();
  }

  using _eo_methods = eo::methods<eo::method<&Type::_eo_String, "String">>;
};

namespace detail {

  // Whether a field or method name is exported. A name that starts with an upper-case letter other
  // than an ASCII one, which Go also exports, is not covered yet, as in fmt.
  inline auto is_exported(std::string_view name) -> bool {
    return !name.empty() && name[0] >= 'A' && name[0] <= 'Z';
  }

  // The package of a named type, from its Go name, which names the package and not its path.
  inline auto package_of(std::string_view type_name) -> std::string {
    auto dot = type_name.find('.');
    return dot == std::string_view::npos ? std::string{} : std::string{type_name.substr(0, dot)};
  }

} // namespace detail

// Go's reflect.StructField. PkgPath is the package's name, which Eo's type names keep, rather than
// its import path.
struct StructField {
  static constexpr std::string_view _eo_name = "reflect.StructField";

  std::string Name{};
  std::string PkgPath{};
  reflect::Type Type{};
  StructTag Tag{};
  uint64_t Offset{};
  slice<int64_t> Index{};
  bool Anonymous{};

  static auto IsExported(StructField f) -> bool {
    return f.PkgPath.empty();
  }

  using _eo_methods = eo::methods<eo::method<&StructField::IsExported, "IsExported">>;
};

inline auto Type::Field(int64_t i) const -> StructField {
  if (t()->kind != eo::detail::rk_struct) {
    eo::panic("reflect: Field of non-struct type " + String());
  }
  auto fields = t()->fields();
  if (i < 0 || i >= static_cast<int64_t>(fields.size())) {
    eo::panic("reflect: Field index out of bounds");
  }
  const auto& f = fields[static_cast<std::size_t>(i)];
  auto out = StructField{};
  out.Name = std::string{f.name};
  if (!detail::is_exported(f.name)) {
    out.PkgPath = detail::package_of(t()->string());
  }
  out.Type = reflect::Type{f.type()};
  out.Tag = StructTag{std::string{f.tag}};
  if (t()->make != nullptr) {
    auto object = t()->make();
    auto* base = static_cast<char*>(t()->rep_addr(object.get()));
    out.Offset = static_cast<uint64_t>(static_cast<char*>(f.address(base)) - base);
  }
  out.Index = slice<int64_t>::of({i});
  out.Anonymous = f.embedded;
  return out;
}

inline auto Type::NumMethod() const -> int64_t {
  if (t()->kind == eo::detail::rk_interface) {
    return static_cast<int64_t>(t()->imethods().size());
  }
  const auto* d = t()->desc();
  if (d == nullptr) {
    return 0;
  }
  return static_cast<int64_t>(
    std::ranges::count_if(d->methods, [](const auto* m) { return detail::is_exported(m->name); }));
}

struct MapIter;

namespace detail {

  inline constexpr uint8_t flag_addr = 1;
  // Go's flagStickyRO and flagEmbedRO. A value read through an unexported embedded field has
  // flag_embed_ro, which Value.Field drops, so the exported fields promoted from it stay settable.
  inline constexpr uint8_t flag_sticky_ro = 2;
  inline constexpr uint8_t flag_embed_ro = 4;
  inline constexpr uint8_t flag_ro = flag_sticky_ro | flag_embed_ro;

  // Go's flag.ro: either read-only bit becomes the sticky one.
  constexpr auto ro(uint8_t flag) noexcept -> uint8_t {
    return (flag & flag_ro) != 0 ? flag_sticky_ro : 0;
  }

  [[noreturn]] inline void panic_value_error(std::string method, uint8_t kind) {
    eo::panic(eo::New<ValueError>(ValueError{std::move(method), Kind{kind}}));
  }

  [[noreturn]] inline void panic_not_covered(std::string what) {
    eo::panic("reflect: " + what + " is not covered by Eo");
  }

} // namespace detail

// Go's reflect.Value.
struct Value {
  static constexpr std::string_view _eo_name = "reflect.Value";

  const eo::detail::rtype* _eo_typ = nullptr;
  // The value's storage, shared with its owner.
  std::shared_ptr<void> _eo_ptr;
  uint8_t _eo_flag = 0;

  static auto Kind(Value v) -> reflect::Kind {
    return reflect::Kind{v._eo_typ == nullptr ? eo::detail::rk_invalid : v._eo_typ->kind};
  }

  static auto Type(Value v) -> reflect::Type {
    if (v._eo_typ == nullptr) {
      detail::panic_value_error("reflect.Value.Type", eo::detail::rk_invalid);
    }
    return reflect::Type{v._eo_typ};
  }

  static auto IsValid(Value v) -> bool {
    return v._eo_typ != nullptr;
  }

  static auto CanAddr(Value v) -> bool {
    return (v._eo_flag & detail::flag_addr) != 0;
  }

  static auto CanSet(Value v) -> bool {
    return (v._eo_flag & (detail::flag_addr | detail::flag_ro)) == detail::flag_addr;
  }

  static auto CanInterface(Value v) -> bool {
    if (v._eo_typ == nullptr) {
      detail::panic_value_error("reflect.Value.CanInterface", eo::detail::rk_invalid);
    }
    return (v._eo_flag & detail::flag_ro) == 0;
  }

  static auto Interface(Value v) -> any {
    if (v._eo_typ == nullptr) {
      detail::panic_value_error("reflect.Value.Interface", eo::detail::rk_invalid);
    }
    if ((v._eo_flag & detail::flag_ro) != 0) {
      eo::panic("reflect.Value.Interface: cannot return value obtained from unexported field or method");
    }
    return pack(v);
  }

  static auto IsNil(Value v) -> bool {
    switch (kind(v)) {
    case eo::detail::rk_chan:
    case eo::detail::rk_func:
    case eo::detail::rk_map:
    case eo::detail::rk_pointer:
    case eo::detail::rk_interface:
    case eo::detail::rk_slice:
      return v._eo_typ->is_nil(v._eo_ptr.get());
    default:
      detail::panic_value_error("reflect.Value.IsNil", kind(v));
    }
  }

  static auto IsZero(Value v) -> bool {
    if (v._eo_typ == nullptr) {
      detail::panic_value_error("reflect.Value.IsZero", eo::detail::rk_invalid);
    }
    return v._eo_typ->is_zero(v._eo_ptr.get());
  }

  static auto Elem(Value v) -> Value {
    switch (kind(v)) {
    case eo::detail::rk_interface: {
      auto [type, value] = v._eo_typ->iface_get(v._eo_ptr.get());
      if (type == nullptr) {
        return Value{};
      }
      return Value{type->reflect(), std::const_pointer_cast<void>(value), detail::ro(v._eo_flag)};
    }
    case eo::detail::rk_pointer: {
      auto target = v._eo_typ->load(v._eo_ptr.get());
      if (target == nullptr) {
        return Value{};
      }
      return Value{v._eo_typ->elem(),
        std::move(target),
        static_cast<uint8_t>(detail::flag_addr | (v._eo_flag & detail::flag_ro))};
    }
    default:
      detail::panic_value_error("reflect.Value.Elem", kind(v));
    }
  }

  static auto Addr(Value v) -> Value {
    if ((v._eo_flag & detail::flag_addr) == 0) {
      eo::panic("reflect.Value.Addr of unaddressable value");
    }
    const auto* pt = eo::detail::pointer_type(v._eo_typ);
    if (pt == nullptr) {
      detail::panic_not_covered("a pointer to " + v._eo_typ->string());
    }
    auto slot = pt->make();
    pt->store(slot.get(), v._eo_ptr);
    return Value{pt, std::move(slot), static_cast<uint8_t>(v._eo_flag & detail::flag_ro)};
  }

  static auto NumField(Value v) -> int64_t {
    must_be(v, eo::detail::rk_struct, "reflect.Value.NumField");
    return static_cast<int64_t>(v._eo_typ->fields().size());
  }

  static auto Field(Value v, int64_t i) -> Value {
    must_be(v, eo::detail::rk_struct, "reflect.Value.Field");
    auto fields = v._eo_typ->fields();
    if (i < 0 || i >= static_cast<int64_t>(fields.size())) {
      eo::panic("reflect: Field index out of range");
    }
    const auto& f = fields[static_cast<std::size_t>(i)];
    auto* address = f.address(v._eo_typ->rep_addr(v._eo_ptr.get()));
    auto flag = static_cast<uint8_t>(v._eo_flag & (detail::flag_addr | detail::flag_sticky_ro));
    if (!detail::is_exported(f.name)) {
      flag |= f.embedded ? detail::flag_embed_ro : detail::flag_sticky_ro;
    }
    return Value{f.type(), std::shared_ptr<void>(v._eo_ptr, address), flag};
  }

  static auto FieldByIndex(Value v, slice<int64_t> index) -> Value {
    if (index.size() == 1) {
      return Field(v, index.data()[0]);
    }
    must_be(v, eo::detail::rk_struct, "reflect.Value.FieldByIndex");
    for (auto i = int64_t{0}; i < index.size(); ++i) {
      if (i > 0 && kind(v) == eo::detail::rk_pointer && v._eo_typ->elem()->kind == eo::detail::rk_struct) {
        if (IsNil(v)) {
          eo::panic("reflect: indirection through nil pointer to embedded struct");
        }
        v = Elem(v);
      }
      v = Field(v, index.data()[i]);
    }
    return v;
  }

  static auto Len(Value v) -> int64_t {
    switch (kind(v)) {
    case eo::detail::rk_array:
      return v._eo_typ->len;
    case eo::detail::rk_slice:
      return v._eo_typ->slice_len(v._eo_ptr.get());
    case eo::detail::rk_map:
      return v._eo_typ->map_len(v._eo_ptr.get());
    case eo::detail::rk_string:
      return static_cast<int64_t>(v._eo_typ->get_string(v._eo_ptr.get()).size());
    case eo::detail::rk_pointer:
      if (v._eo_typ->elem()->kind == eo::detail::rk_array) {
        return v._eo_typ->elem()->len;
      }
      eo::panic("reflect: call of reflect.Value.Len on ptr to non-array Value");
    default:
      detail::panic_value_error("reflect.Value.Len", kind(v));
    }
  }

  static auto Cap(Value v) -> int64_t {
    switch (kind(v)) {
    case eo::detail::rk_array:
      return v._eo_typ->len;
    case eo::detail::rk_slice:
      return v._eo_typ->slice_cap(v._eo_ptr.get());
    case eo::detail::rk_pointer:
      if (v._eo_typ->elem()->kind == eo::detail::rk_array) {
        return v._eo_typ->elem()->len;
      }
      eo::panic("reflect: call of reflect.Value.Cap on ptr to non-array Value");
    default:
      detail::panic_value_error("reflect.Value.Cap", kind(v));
    }
  }

  static auto Index(Value v, int64_t i) -> Value {
    switch (kind(v)) {
    case eo::detail::rk_array: {
      if (i < 0 || i >= v._eo_typ->len) {
        eo::panic("reflect: array index out of range");
      }
      auto* address = v._eo_typ->array_at(v._eo_ptr.get(), i);
      return Value{v._eo_typ->elem(),
        std::shared_ptr<void>(v._eo_ptr, address),
        static_cast<uint8_t>((v._eo_flag & detail::flag_addr) | detail::ro(v._eo_flag))};
    }
    case eo::detail::rk_slice: {
      if (i < 0 || i >= v._eo_typ->slice_len(v._eo_ptr.get())) {
        eo::panic("reflect: slice index out of range");
      }
      return Value{v._eo_typ->elem(),
        v._eo_typ->slice_at(v._eo_ptr.get(), i),
        static_cast<uint8_t>(detail::flag_addr | detail::ro(v._eo_flag))};
    }
    case eo::detail::rk_string: {
      auto s = v._eo_typ->get_string(v._eo_ptr.get());
      if (i < 0 || i >= static_cast<int64_t>(s.size())) {
        eo::panic("reflect: string index out of range");
      }
      return Value{eo::detail::rtype_of<uint8_t>(),
        std::make_shared<uint8_t>(static_cast<uint8_t>(s[static_cast<std::size_t>(i)])),
        detail::ro(v._eo_flag)};
    }
    default:
      detail::panic_value_error("reflect.Value.Index", kind(v));
    }
  }

  static auto Bool(Value v) -> bool {
    must_be(v, eo::detail::rk_bool, "reflect.Value.Bool");
    return v._eo_typ->get_bool(v._eo_ptr.get());
  }

  static auto Int(Value v) -> int64_t {
    if (!is_int(kind(v))) {
      detail::panic_value_error("reflect.Value.Int", kind(v));
    }
    return v._eo_typ->get_int(v._eo_ptr.get());
  }

  static auto Uint(Value v) -> uint64_t {
    if (!is_uint(kind(v))) {
      detail::panic_value_error("reflect.Value.Uint", kind(v));
    }
    return v._eo_typ->get_uint(v._eo_ptr.get());
  }

  static auto Float(Value v) -> double {
    if (!is_float(kind(v))) {
      detail::panic_value_error("reflect.Value.Float", kind(v));
    }
    return v._eo_typ->get_float(v._eo_ptr.get());
  }

  // Go's Value.String, which returns a string that names the type for a Value of another kind, rather
  // than panicking, as fmt treats Values specially.
  static auto String(Value v) -> std::string {
    if (kind(v) == eo::detail::rk_string) {
      return v._eo_typ->get_string(v._eo_ptr.get());
    }
    if (v._eo_typ == nullptr) {
      return "<invalid Value>";
    }
    return "<" + v._eo_typ->string() + " Value>";
  }

  static auto Bytes(Value v) -> slice<uint8_t> {
    switch (kind(v)) {
    case eo::detail::rk_slice:
      if (v._eo_typ->elem()->kind != eo::detail::rk_uint8) {
        eo::panic("reflect.Value.Bytes of non-byte slice");
      }
      if (v._eo_typ->get_bytes == nullptr) {
        detail::panic_not_covered("Bytes of " + v._eo_typ->string());
      }
      return v._eo_typ->get_bytes(v._eo_ptr.get());
    case eo::detail::rk_array: {
      if (v._eo_typ->elem()->kind != eo::detail::rk_uint8) {
        eo::panic("reflect.Value.Bytes of non-byte array");
      }
      if ((v._eo_flag & detail::flag_addr) == 0) {
        eo::panic("reflect.Value.Bytes of unaddressable byte array");
      }
      if (v._eo_typ->elem() != eo::detail::rtype_of<uint8_t>()) {
        detail::panic_not_covered("Bytes of " + v._eo_typ->string());
      }
      auto n = v._eo_typ->len;
      auto* first = static_cast<uint8_t*>(v._eo_typ->rep_addr(v._eo_ptr.get()));
      return slice<uint8_t>::of_array(eo::detail::ptr_access::make(std::shared_ptr<uint8_t>(v._eo_ptr, first)), n);
    }
    default:
      detail::panic_value_error("reflect.Value.Bytes", kind(v));
    }
  }

  static void Set(Value v, Value x) {
    must_be_assignable(v, "reflect.Value.Set");
    must_be_exported(x, "reflect.Value.Set");
    assign_to(x, "reflect.Set", v._eo_typ, v._eo_ptr.get());
  }

  static void SetBool(Value v, bool x) {
    must_be_assignable(v, "reflect.Value.SetBool");
    must_be(v, eo::detail::rk_bool, "reflect.Value.SetBool");
    v._eo_typ->set_bool(v._eo_ptr.get(), x);
  }

  static void SetInt(Value v, int64_t x) {
    must_be_assignable(v, "reflect.Value.SetInt");
    if (!is_int(kind(v))) {
      detail::panic_value_error("reflect.Value.SetInt", kind(v));
    }
    v._eo_typ->set_int(v._eo_ptr.get(), x);
  }

  static void SetUint(Value v, uint64_t x) {
    must_be_assignable(v, "reflect.Value.SetUint");
    if (!is_uint(kind(v))) {
      detail::panic_value_error("reflect.Value.SetUint", kind(v));
    }
    v._eo_typ->set_uint(v._eo_ptr.get(), x);
  }

  static void SetFloat(Value v, double x) {
    must_be_assignable(v, "reflect.Value.SetFloat");
    if (!is_float(kind(v))) {
      detail::panic_value_error("reflect.Value.SetFloat", kind(v));
    }
    v._eo_typ->set_float(v._eo_ptr.get(), x);
  }

  static void SetString(Value v, std::string x) {
    must_be_assignable(v, "reflect.Value.SetString");
    must_be(v, eo::detail::rk_string, "reflect.Value.SetString");
    v._eo_typ->set_string(v._eo_ptr.get(), std::move(x));
  }

  static void SetBytes(Value v, slice<uint8_t> x) {
    must_be_assignable(v, "reflect.Value.SetBytes");
    must_be(v, eo::detail::rk_slice, "reflect.Value.SetBytes");
    if (v._eo_typ->elem()->kind != eo::detail::rk_uint8) {
      eo::panic("reflect.Value.SetBytes of non-byte slice");
    }
    if (v._eo_typ->set_bytes == nullptr) {
      detail::panic_not_covered("SetBytes of " + v._eo_typ->string());
    }
    v._eo_typ->set_bytes(v._eo_ptr.get(), std::move(x));
  }

  static void SetZero(Value v) {
    must_be_assignable(v, "reflect.Value.SetZero");
    auto zero = v._eo_typ->make();
    v._eo_typ->assign(v._eo_ptr.get(), zero.get());
  }

  static void SetLen(Value v, int64_t n) {
    must_be_assignable(v, "reflect.Value.SetLen");
    must_be(v, eo::detail::rk_slice, "reflect.Value.SetLen");
    if (n < 0 || n > v._eo_typ->slice_cap(v._eo_ptr.get())) {
      eo::panic("reflect: slice length out of range in SetLen");
    }
    v._eo_typ->slice_set_len(v._eo_ptr.get(), n);
  }

  static void Grow(Value v, int64_t n) {
    must_be_assignable(v, "reflect.Value.Grow");
    must_be(v, eo::detail::rk_slice, "reflect.Value.Grow");
    if (n < 0) {
      eo::panic("reflect.Value.Grow: negative len");
    }
    if (v._eo_typ->slice_len(v._eo_ptr.get()) + n < 0) {
      eo::panic("reflect.Value.Grow: slice overflow");
    }
    v._eo_typ->slice_grow(v._eo_ptr.get(), n);
  }

  static auto MapIndex(Value v, Value key) -> Value {
    must_be(v, eo::detail::rk_map, "reflect.Value.MapIndex");
    auto k = converted(key, "reflect.Value.MapIndex", v._eo_typ->key());
    const auto* elem = v._eo_typ->elem();
    auto found = elem->make();
    if (!v._eo_typ->map_get(v._eo_ptr.get(), k.get(), found.get())) {
      return Value{};
    }
    return Value{elem, std::move(found), detail::ro(v._eo_flag | key._eo_flag)};
  }

  static auto MapKeys(Value v) -> slice<Value> {
    must_be(v, eo::detail::rk_map, "reflect.Value.MapKeys");
    auto entries = map_entries(v);
    if (entries.empty()) {
      return slice<Value>::allocate(0, 0);
    }
    auto out = slice<Value>::allocate(static_cast<int64_t>(entries.size()), static_cast<int64_t>(entries.size()));
    for (std::size_t i = 0; i < entries.size(); ++i) {
      out.data()[i] = Value{v._eo_typ->key(), std::move(entries[i].first), detail::ro(v._eo_flag)};
    }
    return out;
  }

  static auto MapRange(Value v) -> ptr<MapIter>;

  static void SetMapIndex(Value v, Value key, Value elem) {
    must_be(v, eo::detail::rk_map, "reflect.Value.SetMapIndex");
    must_be_exported(v, "reflect.Value.SetMapIndex");
    must_be_exported(key, "reflect.Value.SetMapIndex");
    auto k = converted(key, "reflect.Value.SetMapIndex", v._eo_typ->key());
    if (elem._eo_typ == nullptr) {
      v._eo_typ->map_delete(v._eo_ptr.get(), k.get());
      return;
    }
    must_be_exported(elem, "reflect.Value.SetMapIndex");
    auto e = converted(elem, "reflect.Value.SetMapIndex", v._eo_typ->elem());
    v._eo_typ->map_set(v._eo_ptr.get(), k.get(), e.get());
  }

  static auto NumMethod(Value v) -> int64_t {
    if (v._eo_typ == nullptr) {
      detail::panic_value_error("reflect.Value.NumMethod", eo::detail::rk_invalid);
    }
    return reflect::Type{v._eo_typ}.NumMethod();
  }

  static auto OverflowInt(Value v, int64_t x) -> bool {
    if (!is_int(kind(v))) {
      detail::panic_value_error("reflect.Value.OverflowInt", kind(v));
    }
    auto bits = reflect::Type{v._eo_typ}.Bits();
    auto trunc = static_cast<int64_t>(static_cast<uint64_t>(x) << (64 - bits)) >> (64 - bits);
    return x != trunc;
  }

  static auto OverflowUint(Value v, uint64_t x) -> bool {
    if (!is_uint(kind(v))) {
      detail::panic_value_error("reflect.Value.OverflowUint", kind(v));
    }
    auto bits = reflect::Type{v._eo_typ}.Bits();
    auto trunc = (x << (64 - bits)) >> (64 - bits);
    return x != trunc;
  }

  static auto OverflowFloat(Value v, double x) -> bool {
    switch (kind(v)) {
    case eo::detail::rk_float32: {
      auto a = x < 0 ? -x : x;
      return FLT_MAX < a && a <= DBL_MAX;
    }
    case eo::detail::rk_float64:
      return false;
    default:
      detail::panic_value_error("reflect.Value.OverflowFloat", kind(v));
    }
  }

  // Go's Value.Equal, from Go 1.26's value.go.
  static auto Equal(Value v, Value u) -> bool {
    if (kind(v) == eo::detail::rk_interface) {
      v = Elem(v);
    }
    if (kind(u) == eo::detail::rk_interface) {
      u = Elem(u);
    }
    if (!IsValid(v) || !IsValid(u)) {
      return IsValid(v) == IsValid(u);
    }
    if (kind(v) != kind(u) || v._eo_typ != u._eo_typ) {
      return false;
    }
    switch (kind(v)) {
    case eo::detail::rk_array: {
      auto vl = Len(v);
      if (vl == 0) {
        if (v._eo_typ->elem()->equal == nullptr) {
          break;
        }
        return true;
      }
      for (auto i = int64_t{0}; i < vl; i++) {
        if (!Equal(Index(v, i), Index(u, i))) {
          return false;
        }
      }
      return true;
    }
    case eo::detail::rk_struct: {
      auto nf = NumField(v);
      for (auto i = int64_t{0}; i < nf; i++) {
        if (!Equal(Field(v, i), Field(u, i))) {
          return false;
        }
      }
      return true;
    }
    case eo::detail::rk_func:
    case eo::detail::rk_map:
    case eo::detail::rk_slice:
      break;
    default:
      return v._eo_typ->equal(v._eo_ptr.get(), u._eo_ptr.get());
    }
    eo::panic("reflect.Value.Equal: values of type " + v._eo_typ->string() + " are not comparable");
  }

  // Go's Value.Pointer, for a pointer, a slice, or a map: the address of the target, of the first
  // element, or of the table, as a uintptr_t. A map's address is its table's, so a map and its copies
  // report the same one, as in Go.
  static auto Pointer(Value v) -> uintptr_t {
    switch (kind(v)) {
    case eo::detail::rk_pointer:
      return reinterpret_cast<uintptr_t>(v._eo_typ->load(v._eo_ptr.get()).get());
    case eo::detail::rk_slice:
    case eo::detail::rk_map:
      return reinterpret_cast<uintptr_t>(v._eo_typ->pointer(v._eo_ptr.get()));
    default:
      detail::panic_value_error("reflect.Value.Pointer", kind(v));
    }
  }

  // The implementation, which is not a translation form.

  static auto kind(const Value& v) -> uint8_t {
    return v._eo_typ == nullptr ? eo::detail::rk_invalid : v._eo_typ->kind;
  }

  static auto is_int(uint8_t k) -> bool {
    return k >= eo::detail::rk_int && k <= eo::detail::rk_int64;
  }

  static auto is_uint(uint8_t k) -> bool {
    return k >= eo::detail::rk_uint && k <= eo::detail::rk_uintptr;
  }

  static auto is_float(uint8_t k) -> bool {
    return k == eo::detail::rk_float32 || k == eo::detail::rk_float64;
  }

  static void must_be(const Value& v, uint8_t k, std::string_view method) {
    if (kind(v) != k) {
      detail::panic_value_error(std::string{method}, kind(v));
    }
  }

  static void must_be_exported(const Value& v, std::string_view method) {
    if (v._eo_typ == nullptr) {
      detail::panic_value_error(std::string{method}, eo::detail::rk_invalid);
    }
    if ((v._eo_flag & detail::flag_ro) != 0) {
      eo::panic("reflect: " + std::string{method} + " using value obtained using unexported field");
    }
  }

  static void must_be_assignable(const Value& v, std::string_view method) {
    must_be_exported(v, method);
    if ((v._eo_flag & detail::flag_addr) == 0) {
      eo::panic("reflect: " + std::string{method} + " using unaddressable value");
    }
  }

  // Go's packEface: the value in an interface value, which shares a value that reflect cannot modify
  // and copies one that it can.
  static auto pack(const Value& v) -> any {
    auto out = any{};
    if (v._eo_typ->kind == eo::detail::rk_interface) {
      auto [type, value] = v._eo_typ->iface_get(v._eo_ptr.get());
      out._eo_type = type;
      out._eo_value = std::move(value);
      return out;
    }
    const auto* d = v._eo_typ->desc();
    if (d == nullptr) {
      detail::panic_not_covered("Interface of a value of type " + v._eo_typ->string());
    }
    out._eo_type = d;
    out._eo_value = (v._eo_flag & detail::flag_addr) != 0 ? copy(v) : v._eo_ptr;
    return out;
  }

  // A new copy of the value.
  static auto copy(const Value& v) -> std::shared_ptr<void> {
    auto out = v._eo_typ->make();
    v._eo_typ->assign(out.get(), v._eo_ptr.get());
    return out;
  }

  // Copies of a map's keys and elements.
  static auto map_entries(const Value& m) -> std::vector<std::pair<std::shared_ptr<void>, std::shared_ptr<void>>> {
    struct collector {
      const eo::detail::rtype* key;
      const eo::detail::rtype* elem;
      std::vector<std::pair<std::shared_ptr<void>, std::shared_ptr<void>>> out;
    };
    auto c = collector{m._eo_typ->key(), m._eo_typ->elem(), {}};
    m._eo_typ->map_each(
      m._eo_ptr.get(),
      [](void* context, const void* k, const void* e) {
        auto& c = *static_cast<collector*>(context);
        auto key = c.key->make();
        c.key->assign(key.get(), k);
        auto elem = c.elem->make();
        c.elem->assign(elem.get(), e);
        c.out.emplace_back(std::move(key), std::move(elem));
      },
      &c);
    return std::move(c.out);
  }

  // Go's assignTo: stores x into target, of type dst, or panics if x is not assignable to dst.
  static void assign_to(const Value& x, std::string_view context, const eo::detail::rtype* dst, void* target) {
    if (reflect::Type::directly_assignable(dst, x._eo_typ)) {
      if (dst == x._eo_typ) {
        dst->assign(target, x._eo_ptr.get());
      } else {
        dst->rep_type()->assign(dst->rep_addr(target), x._eo_typ->rep_addr(x._eo_ptr.get()));
      }
      return;
    }
    if (reflect::Type::implements(dst, x._eo_typ)) {
      auto packed = pack(x);
      if (!dst->iface_set(target, packed._eo_type, packed._eo_value)) {
        detail::panic_not_covered("assigning " + x._eo_typ->string() + " to " + dst->string());
      }
      return;
    }
    eo::panic(
      std::string{context} + ": value of type " + x._eo_typ->string() + " is not assignable to type " + dst->string());
  }

  // x as a value of type dst, for a map key or element. Go's MapIndex takes a key obtained through an
  // unexported field, so this does not check where x came from.
  static auto converted(
    const Value& x, std::string_view context, const eo::detail::rtype* dst) -> std::shared_ptr<void> {
    if (x._eo_typ == nullptr) {
      detail::panic_value_error(std::string{context}, eo::detail::rk_invalid);
    }
    if (x._eo_typ == dst) {
      return x._eo_ptr;
    }
    auto out = dst->make();
    assign_to(x, context, dst, out.get());
    return out;
  }
};

// Go's reflect.MapIter. It iterates over a copy of the map's entries, taken by the first Next, so it
// does not see entries added or removed during the iteration, which Go may or may not see.
struct MapIter {
  static constexpr std::string_view _eo_name = "reflect.MapIter";

  reflect::Value m{};
  std::vector<std::pair<std::shared_ptr<void>, std::shared_ptr<void>>> entries{};
  bool started = false;
  std::size_t i = 0;

  static auto Next(ptr<MapIter> iter) -> bool {
    if (!reflect::Value::IsValid(iter->m)) {
      eo::panic("MapIter.Next called on an iterator that does not have an associated map Value");
    }
    if (!iter->started) {
      iter->started = true;
      iter->entries = reflect::Value::map_entries(iter->m);
      iter->i = 0;
    } else {
      if (iter->i >= iter->entries.size()) {
        eo::panic("MapIter.Next called on exhausted iterator");
      }
      ++iter->i;
    }
    return iter->i < iter->entries.size();
  }

  static auto Key(ptr<MapIter> iter) -> reflect::Value {
    if (!iter->started) {
      eo::panic("MapIter.Key called before Next");
    }
    if (iter->i >= iter->entries.size()) {
      eo::panic("MapIter.Key called on exhausted iterator");
    }
    return reflect::Value{iter->m._eo_typ->key(), iter->entries[iter->i].first, detail::ro(iter->m._eo_flag)};
  }

  static auto Value(ptr<MapIter> iter) -> reflect::Value {
    if (!iter->started) {
      eo::panic("MapIter.Value called before Next");
    }
    if (iter->i >= iter->entries.size()) {
      eo::panic("MapIter.Value called on exhausted iterator");
    }
    return reflect::Value{iter->m._eo_typ->elem(), iter->entries[iter->i].second, detail::ro(iter->m._eo_flag)};
  }
};

inline auto Value::MapRange(Value v) -> ptr<MapIter> {
  if (kind(v) != eo::detail::rk_map) {
    detail::panic_value_error("reflect.Value.MapRange", kind(v));
  }
  auto it = eo::New<MapIter>();
  it->m = v;
  return it;
}

// Go's TypeOf: the dynamic type of i, or nil.
inline auto TypeOf(const any& i) -> Type {
  if (i._eo_type == nullptr) {
    return Type{};
  }
  return Type{i._eo_type->reflect()};
}

// Go's TypeFor[T]().
template<typename T>
auto TypeFor() -> Type {
  return Type{eo::detail::rtype_of<T>()};
}

// Go's PointerTo and its old name PtrTo. A pointer to a pointer to a pointer is not covered.
inline auto PointerTo(Type t) -> Type {
  const auto* pt = eo::detail::pointer_type(t.t());
  if (pt == nullptr) {
    detail::panic_not_covered("PointerTo of " + t.String());
  }
  return Type{pt};
}

inline auto PtrTo(Type t) -> Type {
  return PointerTo(t);
}

// Go's ValueOf: a Value of the dynamic value of i, which cannot be set.
inline auto ValueOf(const any& i) -> Value {
  if (i._eo_type == nullptr) {
    return Value{};
  }
  return Value{i._eo_type->reflect(), std::const_pointer_cast<void>(i._eo_value), 0};
}

// Go's New: a pointer to a new zero value of type t.
inline auto New(Type t) -> Value {
  if (t == nullptr) {
    eo::panic("reflect: New(nil)");
  }
  const auto* pt = eo::detail::pointer_type(t._eo_t);
  if (pt == nullptr) {
    detail::panic_not_covered("New of " + t.String());
  }
  auto slot = pt->make();
  pt->store(slot.get(), t._eo_t->make());
  return Value{pt, std::move(slot), 0};
}

// Go's Zero: the zero value of type t, which cannot be set.
inline auto Zero(Type t) -> Value {
  if (t == nullptr) {
    eo::panic("reflect: Zero(nil)");
  }
  return Value{t._eo_t, t._eo_t->make(), 0};
}

inline auto Indirect(Value v) -> Value {
  if (Value::Kind(v) != Pointer) {
    return v;
  }
  return Value::Elem(v);
}

inline auto MakeSlice(Type typ, int64_t len, int64_t cap) -> Value {
  if (typ.Kind() != Slice) {
    eo::panic("reflect.MakeSlice of non-slice type");
  }
  if (len < 0) {
    eo::panic("reflect.MakeSlice: negative len");
  }
  if (cap < 0) {
    eo::panic("reflect.MakeSlice: negative cap");
  }
  if (len > cap) {
    eo::panic("reflect.MakeSlice: len > cap");
  }
  auto object = typ._eo_t->make();
  typ._eo_t->slice_make(object.get(), len, cap);
  return Value{typ._eo_t, std::move(object), 0};
}

inline auto MakeMapWithSize(Type typ, int64_t n) -> Value {
  if (typ.Kind() != Map) {
    eo::panic("reflect.MakeMapWithSize of non-map type");
  }
  auto object = typ._eo_t->make();
  typ._eo_t->map_make(object.get(), n);
  return Value{typ._eo_t, std::move(object), 0};
}

inline auto MakeMap(Type typ) -> Value {
  return MakeMapWithSize(typ, 0);
}

// Go's Copy: copies min(dst.Len(), src.Len()) elements from src, an array, slice, or string, into
// dst, an array or slice, which may overlap.
inline auto Copy(Value dst, Value src) -> int64_t {
  auto dk = Value::kind(dst);
  if (dk != eo::detail::rk_array && dk != eo::detail::rk_slice) {
    detail::panic_value_error("reflect.Copy", dk);
  }
  if (dk == eo::detail::rk_array) {
    Value::must_be_assignable(dst, "reflect.Copy");
  }
  Value::must_be_exported(dst, "reflect.Copy");
  auto sk = Value::kind(src);
  auto string_copy = false;
  if (sk != eo::detail::rk_array && sk != eo::detail::rk_slice) {
    string_copy = sk == eo::detail::rk_string && dst._eo_typ->elem()->kind == eo::detail::rk_uint8;
    if (!string_copy) {
      detail::panic_value_error("reflect.Copy", sk);
    }
  }
  Value::must_be_exported(src, "reflect.Copy");
  const auto* de = dst._eo_typ->elem();
  if (!string_copy && src._eo_typ->elem() != de) {
    eo::panic("reflect.Copy: " + de->string() + " != " + src._eo_typ->elem()->string());
  }
  auto n = std::min(Value::Len(dst), Value::Len(src));
  if (string_copy) {
    auto s = src._eo_typ->get_string(src._eo_ptr.get());
    for (auto i = int64_t{0}; i < n; ++i) {
      Value::SetUint(Value::Index(dst, i), static_cast<uint8_t>(s[static_cast<std::size_t>(i)]));
    }
    return n;
  }
  auto element = [](const Value& v, int64_t i) -> void* {
    if (Value::kind(v) == eo::detail::rk_array) {
      return v._eo_typ->array_at(v._eo_ptr.get(), i);
    }
    return v._eo_typ->slice_at(v._eo_ptr.get(), i).get();
  };
  if (n == 0) {
    return 0;
  }
  // Copy backward when dst starts after src's first element and no later than its last, as memmove does.
  auto* d0 = element(dst, 0);
  auto* s0 = element(src, 0);
  if (std::less<void*>{}(s0, d0) && !std::less<void*>{}(element(src, n - 1), d0)) {
    for (auto i = n; i-- > 0;) {
      de->assign(element(dst, i), element(src, i));
    }
  } else {
    for (auto i = int64_t{0}; i < n; ++i) {
      de->assign(element(dst, i), element(src, i));
    }
  }
  return n;
}

// Go 1.25's TypeAssert[T](v): v.Interface().(T) without the conversion to any.
template<typename T>
auto TypeAssert(Value v) -> std::tuple<T, bool> {
  if (v._eo_typ == nullptr) {
    detail::panic_value_error("reflect.TypeAssert", eo::detail::rk_invalid);
  }
  if ((v._eo_flag & detail::flag_ro) != 0) {
    eo::panic("reflect.TypeAssert: cannot return value obtained from unexported field or method");
  }
  if (v._eo_typ->kind == eo::detail::rk_interface || eo::detail::interface_type<T>) {
    auto [x, ok] = eo::type_assert_ok<T>(Value::pack(v));
    return {x, ok};
  }
  if (v._eo_typ != eo::detail::rtype_of<T>()) {
    return {T{}, false};
  }
  return {*static_cast<const T*>(v._eo_ptr.get()), true};
}

// fmt prints a Value operand as the value it holds, as Go's printArg does. A Value inside another
// value, which Go prints by the Value's own fields, is not covered yet.
inline void _eo_print(eo::detail::printer& p, const Value& v, int32_t verb, int depth) {
  if (!Value::IsValid(v)) {
    p.buf += "<invalid reflect.Value>";
    return;
  }
  if (Value::CanInterface(v)) {
    eo::detail::print_arg(p, Value::pack(v), verb, depth);
    return;
  }
  detail::panic_not_covered("printing a Value obtained from an unexported field");
}

} // namespace eo::reflect
