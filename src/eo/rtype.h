// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/fields.h>
#include <eo/interface.h>
#include <eo/struct.h>

#include <boost/pfr.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

// The run-time type information that <eo/reflect.h> reads, as Go's runtime keeps it for every type.
// The descriptor of a type held in an interface points to it, so reflect.ValueOf finds it from any
// interface value. Each type's information is built on first use, and the types it refers to, such as
// an element or a field type, only through functions, so a type may refer to itself through a pointer.
// Values are handled with their types erased: a value is the address of its storage, and the
// operations below are those of its type, applied to its representation, which is its underlying type
// for a defined type.
//
// <eo/panic.h> includes this header last, after <eo/print.h> and <eo/ptr.h>. Depending on which
// header a program includes first, <eo/ptr.h>, <eo/slice.h>, and <eo/map.h> may not be defined yet
// at this point, so the code below reaches them only through names that depend on a template
// parameter, which are looked up where a type's information is built.

namespace eo {

template<typename Sig>
class fn;

template<typename T>
class chan;

namespace detail {

  // Go's reflect.Kind values.
  enum rkind : uint8_t {
    rk_invalid,
    rk_bool,
    rk_int,
    rk_int8,
    rk_int16,
    rk_int32,
    rk_int64,
    rk_uint,
    rk_uint8,
    rk_uint16,
    rk_uint32,
    rk_uint64,
    rk_uintptr,
    rk_float32,
    rk_float64,
    rk_complex64,
    rk_complex128,
    rk_array,
    rk_chan,
    rk_func,
    rk_interface,
    rk_map,
    rk_pointer,
    rk_slice,
    rk_string,
    rk_struct,
    rk_unsafe_pointer,
  };

  using rtype_fn = auto (*)() -> const rtype*;

  // From <eo/slice.h>.
  inline auto next_slice_cap(int64_t new_len, int64_t old_cap) -> int64_t;

  // U, as a type that depends on T.
  template<typename T, typename U>
  struct dependent {
    using type = U;
  };

  template<typename T>
  using ptr_access_for = typename dependent<T, ptr_access>::type;

  // One field of a struct type, with the address of the field in a value of the struct.
  struct rfield {
    std::string_view name;
    rtype_fn type;
    std::string_view tag;
    bool embedded;
    auto (*address)(void* s) -> void*;
  };

  // A type's information as another type's information refers to it: through the function that
  // returns it, for a type known at compile time, or directly, for a pointer type made at run time.
  struct rtype_ref {
    rtype_fn fn = nullptr;
    const rtype* type = nullptr;

    auto operator()() const -> const rtype* {
      return type != nullptr ? type : fn();
    }

    explicit operator bool() const noexcept {
      return fn != nullptr || type != nullptr;
    }
  };

  // The operations of the pointer type *T, kept with T's information, so that reflect.PointerTo can
  // make *T at run time without its information being built at compile time, and those of **T.
  struct rptr_ops {
    auto (*string)() -> std::string;
    auto (*make)() -> std::shared_ptr<void>;
    void (*assign)(void* dst, const void* src);
    auto (*equal)(const void*, const void*) -> bool;
    auto (*is_nil)(const void*) -> bool;
    auto (*load)(const void*) -> std::shared_ptr<void>;
    void (*store)(void*, std::shared_ptr<void>);
    auto (*next)() -> const rptr_ops*;
  };

  struct rtype {
    uint8_t kind = rk_invalid;
    // The descriptor of the type when an interface can hold its values, or nullptr. A pointer type made
    // at run time gets it later (see register_pointer_descriptor).
    mutable std::atomic<const type_descriptor*> descriptor{nullptr};
    auto (*string)() -> std::string = nullptr;
    std::string_view name;
    int64_t len = 0;
    rtype_ref elem;
    rtype_ref key;
    // The pointer type *T: built at compile time when T has methods, so that the methods of *T are
    // known, and otherwise made from ptr_ops on first use and kept in ptr_cache, so that *T is one type.
    rtype_fn ptr_static = nullptr;
    const rptr_ops* ptr_ops = nullptr;
    mutable std::atomic<const rtype*> ptr_cache{nullptr};
    auto (*fields)() -> std::span<const rfield> = nullptr;
    // An interface type's methods, ordered by name.
    auto (*imethods)() -> std::span<const entry_base> = nullptr;
    auto (*equal)(const void*, const void*) -> bool = nullptr;
    // The representation type, which is the type itself unless it is a defined type, and the address
    // of a value's representation.
    rtype_ref rep;
    auto (*rep_addr)(void*) -> void* = nullptr;

    // Every type: a new zero value, and copying a value.
    auto (*make)() -> std::shared_ptr<void> = nullptr;
    void (*assign)(void* dst, const void* src) = nullptr;
    auto (*is_zero)(const void*) -> bool = nullptr;

    // Booleans, numbers, and strings.
    auto (*get_bool)(const void*) -> bool = nullptr;
    void (*set_bool)(void*, bool) = nullptr;
    auto (*get_int)(const void*) -> int64_t = nullptr;
    void (*set_int)(void*, int64_t) = nullptr;
    auto (*get_uint)(const void*) -> uint64_t = nullptr;
    void (*set_uint)(void*, uint64_t) = nullptr;
    auto (*get_float)(const void*) -> double = nullptr;
    void (*set_float)(void*, double) = nullptr;
    auto (*get_string)(const void*) -> std::string = nullptr;
    void (*set_string)(void*, std::string) = nullptr;

    // Pointers, slices, maps, interfaces, functions, and channels.
    auto (*is_nil)(const void*) -> bool = nullptr;

    // Pointers: the target, as a shared pointer that keeps it alive, and storing a new target.
    auto (*load)(const void*) -> std::shared_ptr<void> = nullptr;
    void (*store)(void*, std::shared_ptr<void>) = nullptr;

    // Slices.
    auto (*slice_len)(const void*) -> int64_t = nullptr;
    auto (*slice_cap)(const void*) -> int64_t = nullptr;
    auto (*slice_at)(const void*, int64_t) -> std::shared_ptr<void> = nullptr;
    void (*slice_make)(void*, int64_t len, int64_t cap) = nullptr;
    void (*slice_set_len)(void*, int64_t) = nullptr;
    void (*slice_grow)(void*, int64_t) = nullptr;
    // A []byte, for Bytes and SetBytes.
    auto (*get_bytes)(const void*) -> slice<uint8_t> = nullptr;
    void (*set_bytes)(void*, slice<uint8_t>) = nullptr;

    // Arrays.
    auto (*array_at)(void*, int64_t) -> void* = nullptr;

    // Maps. A key or element is passed as the address of a value of the key or element type, and
    // copied into one: map_get copies the element of a key into out, and map_each calls f with each
    // entry.
    auto (*map_len)(const void*) -> int64_t = nullptr;
    void (*map_make)(void*, int64_t hint) = nullptr;
    auto (*map_get)(const void*, const void* key, void* out) -> bool = nullptr;
    void (*map_set)(void*, const void* key, const void* value) = nullptr;
    void (*map_delete)(void*, const void* key) = nullptr;
    void (
      *map_each)(const void*, void (*f)(void* context, const void* key, const void* value), void* context) = nullptr;

    // Interfaces: the dynamic type and value, and storing one, which fails if the dynamic type lacks
    // a method of the interface.
    auto (*iface_get)(const void*) -> std::pair<const type_descriptor*, std::shared_ptr<const void>> = nullptr;
    auto (*iface_set)(void*, const type_descriptor*, std::shared_ptr<const void>) -> bool = nullptr;

    auto desc() const noexcept -> const type_descriptor* {
      return descriptor.load(std::memory_order_acquire);
    }

    auto rep_type() const -> const rtype* {
      return rep ? rep() : this;
    }
  };

  // The representation of a value of T: the value itself, or for a defined type the value of its
  // underlying type.
  template<typename T>
  struct rep {
    using type = T;

    static auto of(void* p) -> T* {
      return static_cast<T*>(p);
    }
  };

  template<defined_type T>
  struct rep<T> {
    using U = typename T::_eo_underlying;
    using type = typename rep<U>::type;

    static auto of(void* p) -> type* {
      auto* d = static_cast<T*>(p);
      if constexpr (std::is_arithmetic_v<U>) {
        return &d->_eo_value;
      } else {
        return rep<U>::of(static_cast<U*>(d));
      }
    }
  };

  template<typename T>
  using rep_t = typename rep<T>::type;

  template<typename T>
  auto rep_of(const void* p) -> rep_t<T>* {
    return rep<T>::of(const_cast<void*>(p));
  }

  template<typename T>
  inline constexpr bool is_fn_type = false;

  template<typename S>
  inline constexpr bool is_fn_type<fn<S>> = true;

  template<typename T>
  inline constexpr bool is_chan_type = false;

  template<typename E>
  inline constexpr bool is_chan_type<chan<E>> = true;

  template<typename T>
  struct chan_element;

  template<typename E>
  struct chan_element<chan<E>> {
    using type = E;
  };

  template<typename T>
  inline constexpr int pointer_depth = 0;

  template<typename T>
  inline constexpr int pointer_depth<ptr<T>> = 1 + pointer_depth<T>;

  // The kind of a representation type. Go's int and int64 are both int64_t, which has kind Int, and
  // uint and uint64 are both uint64_t, which has kind Uint, as %T names them.
  template<typename R>
  consteval auto kind_of() -> uint8_t {
    if constexpr (interface_type<R>) {
      return rk_interface;
    } else if constexpr (std::is_same_v<R, bool>) {
      return rk_bool;
    } else if constexpr (std::is_same_v<R, int8_t>) {
      return rk_int8;
    } else if constexpr (std::is_same_v<R, int16_t>) {
      return rk_int16;
    } else if constexpr (std::is_same_v<R, int32_t>) {
      return rk_int32;
    } else if constexpr (std::is_same_v<R, int64_t>) {
      return rk_int;
    } else if constexpr (std::is_same_v<R, uint8_t>) {
      return rk_uint8;
    } else if constexpr (std::is_same_v<R, uint16_t>) {
      return rk_uint16;
    } else if constexpr (std::is_same_v<R, uint32_t>) {
      return rk_uint32;
    } else if constexpr (std::is_same_v<R, uint64_t>) {
      return rk_uint;
    } else if constexpr (std::is_same_v<R, float>) {
      return rk_float32;
    } else if constexpr (std::is_same_v<R, double>) {
      return rk_float64;
    } else if constexpr (std::is_same_v<R, std::string>) {
      return rk_string;
    } else if constexpr (is_slice<R>) {
      return rk_slice;
    } else if constexpr (is_std_array<R>) {
      return rk_array;
    } else if constexpr (is_map<R>) {
      return rk_map;
    } else if constexpr (is_ptr<R>) {
      return rk_pointer;
    } else if constexpr (is_fn_type<R>) {
      return rk_func;
    } else if constexpr (is_chan_type<R>) {
      return rk_chan;
    } else if constexpr (std::is_same_v<R, empty_struct> || go_struct<R>) {
      return rk_struct;
    } else {
      return rk_invalid;
    }
  }

  template<typename T>
  auto rtype_string() -> std::string;

  template<typename... A>
  auto type_list() -> std::string {
    auto out = std::string{};
    ((out += (out.empty() ? "" : ", ") + rtype_string<A>()), ...);
    return out;
  }

  template<typename R>
  auto result_string() -> std::string {
    if constexpr (std::is_void_v<R>) {
      return "";
    } else if constexpr (requires { std::tuple_size<R>::value; } && !is_std_array<R>) {
      return [&]<std::size_t... I>(std::index_sequence<I...>) {
        return " (" + type_list<std::tuple_element_t<I, R>...>() + ")";
      }(std::make_index_sequence<std::tuple_size_v<R>>{});
    } else {
      return " " + rtype_string<R>();
    }
  }

  template<typename F>
  struct fn_string;

  template<typename R, typename... A>
  struct fn_string<fn<R(A...)>> {
    static auto get() -> std::string {
      return "func(" + type_list<A...>() + ")" + result_string<R>();
    }
  };

  // The Go type name, as Type.String reports it. A type that an interface can hold has the name that
  // %T prints, and a function or channel type, or a composite of one, is named here.
  template<typename T>
  auto rtype_string() -> std::string {
    using U = std::remove_cv_t<T>;
    if constexpr (requires { U::_eo_name; }) {
      return std::string{U::_eo_name};
    } else if constexpr (is_fn_type<U>) {
      return fn_string<U>::get();
    } else if constexpr (is_chan_type<U>) {
      return "chan " + rtype_string<typename chan_element<U>::type>();
    } else if constexpr (is_ptr<U>) {
      return "*" + rtype_string<typename pointee<U>::type>();
    } else if constexpr (is_std_array<U>) {
      return "[" + std::to_string(std::tuple_size_v<U>) + "]" + rtype_string<typename U::value_type>();
    } else if constexpr (is_slice<U>) {
      return "[]" + rtype_string<typename U::value_type>();
    } else if constexpr (is_map<U>) {
      return "map[" + rtype_string<typename U::key_type>() + "]" + rtype_string<typename U::mapped_type>();
    } else if constexpr (go_type<U>) {
      return type_name<U>();
    } else {
      return "?";
    }
  }

  // The name of a type within its package, as Type.Name reports it: empty for an unnamed type.
  template<typename T>
  constexpr auto rtype_name() -> std::string_view {
    if constexpr (std::is_same_v<T, any>) {
      return "";
    } else if constexpr (requires { T::_eo_name; }) {
      auto s = std::string_view{T::_eo_name};
      auto dot = s.find('.');
      return dot == std::string_view::npos ? s : s.substr(dot + 1);
    } else if constexpr (std::is_same_v<T, bool>) {
      return "bool";
    } else if constexpr (std::is_same_v<T, int8_t>) {
      return "int8";
    } else if constexpr (std::is_same_v<T, int16_t>) {
      return "int16";
    } else if constexpr (std::is_same_v<T, int32_t>) {
      return "int32";
    } else if constexpr (std::is_same_v<T, int64_t>) {
      return "int";
    } else if constexpr (std::is_same_v<T, uint8_t>) {
      return "uint8";
    } else if constexpr (std::is_same_v<T, uint16_t>) {
      return "uint16";
    } else if constexpr (std::is_same_v<T, uint32_t>) {
      return "uint32";
    } else if constexpr (std::is_same_v<T, uint64_t>) {
      return "uint";
    } else if constexpr (std::is_same_v<T, float>) {
      return "float32";
    } else if constexpr (std::is_same_v<T, double>) {
      return "float64";
    } else if constexpr (std::is_same_v<T, std::string>) {
      return "string";
    } else {
      return "";
    }
  }

  template<typename S, std::size_t I>
  using field_type = std::remove_cvref_t<decltype(boost::pfr::get<I>(std::declval<S&>()))>;

  template<typename S, std::size_t I>
  auto field_address(void* s) -> void* {
    return std::addressof(boost::pfr::get<I>(*static_cast<S*>(s)));
  }

  template<typename S, std::size_t I>
  constexpr auto field_info() -> field {
    if constexpr (requires { S::_eo_fields; }) {
      static_assert(std::size(S::_eo_fields) == boost::pfr::tuple_size_v<S>, "_eo_fields lists every field");
      return S::_eo_fields[I];
    } else {
      return {};
    }
  }

  template<typename S>
  auto struct_fields() -> std::span<const rfield> {
    if constexpr (std::is_same_v<S, empty_struct>) {
      return {};
    } else {
      static const auto fields = []<std::size_t... I>(std::index_sequence<I...>) {
        return std::array<rfield, sizeof...(I)>{rfield{field_name<S, I>(),
          &rtype_of<field_type<S, I>>,
          field_info<S, I>().tag,
          field_info<S, I>().embedded,
          &field_address<S, I>}...};
      }(std::make_index_sequence<boost::pfr::tuple_size_v<S>>{});
      return fields;
    }
  }

  template<typename I>
  auto interface_rmethods() -> std::span<const entry_base> {
    static const auto& methods = *new auto([] {
      auto out = std::vector<entry_base>{};
      for (const auto& m : interface_methods<I>::sorted()) {
        out.push_back(entry_base{m.name, m.signature});
      }
      return out;
    }());
    return methods;
  }

  // Go's Value.IsZero: whether a value is its type's zero value, with a float zero when it equals 0, so
  // -0.0 is zero and NaN is not, and an array or struct zero when every element or field is.
  template<typename T>
  auto zero_value(const void* p) -> bool {
    using R = rep_t<T>;
    const R& v = *rep_of<T>(p);
    if constexpr (std::is_arithmetic_v<R>) {
      return v == R{};
    } else if constexpr (std::is_same_v<R, std::string>) {
      return v.empty();
    } else if constexpr (interface_type<R>) {
      return v._eo_type == nullptr;
    } else if constexpr (is_ptr<R> || is_slice<R> || is_map<R>) {
      return v == nullptr;
    } else if constexpr (is_std_array<R>) {
      return std::ranges::all_of(v, [](const auto& e) { return zero_value<typename R::value_type>(&e); });
    } else if constexpr (std::is_same_v<R, empty_struct>) {
      return true;
    } else if constexpr (go_struct<R>) {
      return [&]<std::size_t... I>(std::index_sequence<I...>) {
        return (... && zero_value<field_type<R, I>>(&boost::pfr::get<I>(v)));
      }(std::make_index_sequence<boost::pfr::tuple_size_v<R>>{});
    } else if constexpr (requires { v == nullptr; }) {
      return v == nullptr;
    } else {
      return false;
    }
  }

  // Whether *T is built at compile time: when T has methods, which the method set of *T includes.
  template<typename T>
  inline constexpr bool static_pointer = has_methods<T>;

  // The operations of **T, for those of *T, which stop at a pointer to a pointer.
  template<typename T>
  auto next_pointer_ops() -> const rptr_ops*;

  template<typename T>
  inline constexpr rptr_ops pointer_ops{
    &rtype_string<ptr<T>>,
    []() -> std::shared_ptr<void> { return std::make_shared<ptr<T>>(); },
    [](void* dst, const void* src) { *static_cast<ptr<T>*>(dst) = *static_cast<const ptr<T>*>(src); },
    [](const void* a, const void* b) { return *static_cast<const ptr<T>*>(a) == *static_cast<const ptr<T>*>(b); },
    [](const void* p) { return *static_cast<const ptr<T>*>(p) == nullptr; },
    [](const void* p) -> std::shared_ptr<void> { return ptr_access_for<T>::shared(*static_cast<const ptr<T>*>(p)); },
    [](void* p, std::shared_ptr<void> target) {
      *static_cast<ptr<T>*>(p) = ptr_access_for<T>::make(std::static_pointer_cast<T>(std::move(target)));
    },
    &next_pointer_ops<T>,
  };

  template<typename T>
  auto next_pointer_ops() -> const rptr_ops* {
    if constexpr (pointer_depth<T> < 1 && !static_pointer<ptr<T>>) {
      return &pointer_ops<ptr<T>>;
    } else {
      return nullptr;
    }
  }

  // The information of type T, built in r.
  template<typename T>
  void fill_rtype(rtype& r) {
    using R = rep_t<T>;
    r.kind = kind_of<R>();
    if constexpr (go_type<T> && !interface_type<T>) {
      r.descriptor.store(&descriptor<T>, std::memory_order_relaxed);
    }
    r.string = &rtype_string<T>;
    r.name = rtype_name<T>();
    if constexpr (static_pointer<T>) {
      r.ptr_static = &rtype_of<ptr<T>>;
    } else if constexpr (pointer_depth<T> < 2) {
      r.ptr_ops = &pointer_ops<T>;
    }
    r.equal = equality_of<T>();
    if constexpr (!std::is_same_v<R, T>) {
      r.rep = rtype_ref{&rtype_of<R>};
    }
    r.rep_addr = [](void* p) -> void* { return rep<T>::of(p); };
    if constexpr (std::is_default_constructible_v<T>) {
      r.make = []() -> std::shared_ptr<void> { return std::make_shared<T>(); };
    }
    if constexpr (std::is_copy_assignable_v<T>) {
      r.assign = [](void* dst, const void* src) { *static_cast<T*>(dst) = *static_cast<const T*>(src); };
    }
    r.is_zero = &zero_value<T>;

    if constexpr (std::is_same_v<R, bool>) {
      r.get_bool = [](const void* p) { return *rep_of<T>(p); };
      r.set_bool = [](void* p, bool x) { *rep<T>::of(p) = x; };
    } else if constexpr (std::is_integral_v<R> && std::is_signed_v<R>) {
      r.get_int = [](const void* p) { return static_cast<int64_t>(*rep_of<T>(p)); };
      r.set_int = [](void* p, int64_t x) { *rep<T>::of(p) = static_cast<R>(x); };
    } else if constexpr (std::is_integral_v<R>) {
      r.get_uint = [](const void* p) { return static_cast<uint64_t>(*rep_of<T>(p)); };
      r.set_uint = [](void* p, uint64_t x) { *rep<T>::of(p) = static_cast<R>(x); };
    } else if constexpr (std::is_floating_point_v<R>) {
      r.get_float = [](const void* p) { return static_cast<double>(*rep_of<T>(p)); };
      r.set_float = [](void* p, double x) { *rep<T>::of(p) = static_cast<R>(x); };
    } else if constexpr (std::is_same_v<R, std::string>) {
      r.get_string = [](const void* p) { return *rep_of<T>(p); };
      r.set_string = [](void* p, std::string x) { *rep<T>::of(p) = std::move(x); };
    } else if constexpr (is_ptr<R>) {
      using E = typename pointee<R>::type;
      r.elem = rtype_ref{&rtype_of<E>};
      r.is_nil = [](const void* p) { return *rep_of<T>(p) == nullptr; };
      r.load = [](const void* p) -> std::shared_ptr<void> { return ptr_access_for<T>::shared(*rep_of<T>(p)); };
      r.store = [](void* p, std::shared_ptr<void> target) {
        *rep<T>::of(p) = ptr_access_for<T>::make(std::static_pointer_cast<E>(std::move(target)));
      };
    } else if constexpr (is_slice<R>) {
      using E = typename R::value_type;
      r.elem = rtype_ref{&rtype_of<E>};
      r.is_nil = [](const void* p) { return *rep_of<T>(p) == nullptr; };
      r.slice_len = [](const void* p) { return rep_of<T>(p)->size(); };
      r.slice_cap = [](const void* p) { return rep_of<T>(p)->capacity(); };
      r.slice_at = [](const void* p, int64_t i) -> std::shared_ptr<void> {
        const auto& s = *rep_of<T>(p);
        return ptr_access_for<T>::shared(ptr<E>(s, s.data() + i));
      };
      r.slice_make = [](void* p, int64_t len, int64_t cap) { *rep<T>::of(p) = R::allocate(len, cap); };
      r.slice_set_len = [](void* p, int64_t n) {
        auto& s = *rep<T>::of(p);
        s = s.view(0, n, s.capacity());
      };
      r.slice_grow = [](void* p, int64_t n) {
        auto& s = *rep<T>::of(p);
        auto len = s.size();
        if (len + n > s.capacity()) {
          auto out = R::allocate(len, next_slice_cap(len + n, s.capacity()));
          std::copy(s.begin(), s.end(), out.data());
          s = out;
        }
      };
      if constexpr (std::is_same_v<E, uint8_t>) {
        r.get_bytes = [](const void* p) { return *rep_of<T>(p); };
        r.set_bytes = [](void* p, R b) { *rep<T>::of(p) = std::move(b); };
      }
    } else if constexpr (is_std_array<R>) {
      using E = typename R::value_type;
      r.elem = rtype_ref{&rtype_of<E>};
      r.len = static_cast<int64_t>(std::tuple_size_v<R>);
      r.array_at = [](void* p, int64_t i) -> void* { return &(*rep<T>::of(p))[static_cast<std::size_t>(i)]; };
    } else if constexpr (is_map<R>) {
      using K = typename R::key_type;
      using V = typename R::mapped_type;
      r.elem = rtype_ref{&rtype_of<V>};
      r.key = rtype_ref{&rtype_of<K>};
      r.is_nil = [](const void* p) { return *rep_of<T>(p) == nullptr; };
      r.map_len = [](const void* p) { return rep_of<T>(p)->size(); };
      r.map_make = [](void* p, int64_t hint) {
        auto m = R::make();
        if (hint > 0) {
          constexpr auto max_reserve = int64_t{1} << 16;
          m.table()->entries.reserve(static_cast<std::size_t>(std::min(hint, max_reserve)));
        }
        *rep<T>::of(p) = std::move(m);
      };
      r.map_get = [](const void* p, const void* k, void* out) {
        if (auto* v = rep_of<T>(p)->find(*static_cast<const K*>(k))) {
          *static_cast<V*>(out) = *v;
          return true;
        }
        return false;
      };
      r.map_set = [](void* p, const void* k, const void* v) {
        rep<T>::of(p)->entry(*static_cast<const K*>(k)) = *static_cast<const V*>(v);
      };
      // A NaN key is never found, so erase removes nothing for one, as delete does.
      r.map_delete = [](void* p, const void* k) {
        if (auto* t = rep<T>::of(p)->table()) {
          t->entries.erase(*static_cast<const K*>(k));
        }
      };
      r.map_each = [](const void* p, void (*f)(void*, const void*, const void*), void* context) {
        if (auto* t = rep_of<T>(p)->table()) {
          t->for_each([&](const K& k, const V& v) { f(context, &k, &v); });
        }
      };
    } else if constexpr (interface_type<R>) {
      r.imethods = &interface_rmethods<R>;
      r.is_nil = [](const void* p) { return rep_of<T>(p)->_eo_type == nullptr; };
      r.iface_get = [](const void* p) -> std::pair<const type_descriptor*, std::shared_ptr<const void>> {
        const auto& v = *rep_of<T>(p);
        return {v._eo_type, v._eo_value};
      };
      r.iface_set = [](void* p, const type_descriptor* type, std::shared_ptr<const void> value) -> bool {
        auto& v = *rep<T>::of(p);
        if (type == nullptr) {
          v = R{};
          return true;
        }
        auto missing = std::string_view{};
        auto table = dynamic_table<R>(type, &missing);
        if (table == nullptr) {
          return false;
        }
        v._eo_type = type;
        v._eo_value = std::move(value);
        v._eo_table = table->data();
        return true;
      };
    } else if constexpr (is_chan_type<R>) {
      r.elem = rtype_ref{&rtype_of<typename chan_element<R>::type>};
      r.is_nil = [](const void* p) { return *rep_of<T>(p) == nullptr; };
    } else if constexpr (is_fn_type<R>) {
      r.is_nil = [](const void* p) { return *rep_of<T>(p) == nullptr; };
    } else if constexpr (std::is_same_v<R, empty_struct> || go_struct<R>) {
      r.fields = &struct_fields<R>;
    }
  }

  // The pointer type *T for T's information elem, or nullptr for a pointer to a pointer to a pointer,
  // which is not covered.
  inline auto pointer_type(const rtype* elem) -> const rtype* {
    if (elem->ptr_static != nullptr) {
      return elem->ptr_static();
    }
    if (const auto* cached = elem->ptr_cache.load(std::memory_order_acquire)) {
      return cached;
    }
    const auto* ops = elem->ptr_ops;
    if (ops == nullptr) {
      return nullptr;
    }
    // Never freed, as the information of a type built at compile time is never destroyed.
    auto* r = new rtype{};
    r->kind = rk_pointer;
    r->string = ops->string;
    r->elem = rtype_ref{nullptr, elem};
    r->ptr_ops = ops->next();
    r->equal = ops->equal;
    r->rep_addr = [](void* p) -> void* { return p; };
    r->make = ops->make;
    r->assign = ops->assign;
    r->is_zero = ops->is_nil;
    r->is_nil = ops->is_nil;
    r->load = ops->load;
    r->store = ops->store;
    const rtype* expected = nullptr;
    if (!elem->ptr_cache.compare_exchange_strong(expected, r, std::memory_order_acq_rel)) {
      delete r;
      return expected;
    }
    return r;
  }

  template<typename T>
  struct rtype_holder {
    rtype r;

    rtype_holder() {
      fill_rtype<T>(r);
    }
  };

  template<typename T>
  auto rtype_of() -> const rtype*;

  // Runs at program start for each pointer type *T that the program converts to an interface, where T
  // has no methods, so that the run-time pointer type has a descriptor before any reflect call.
  template<typename D>
  void register_pointer_descriptor() {
    static_cast<void>(rtype_of<D>());
  }

  // Whether *E, for T = *E, is the pointer type that pointer_type makes from E's information: when E
  // has no methods, unless T is a pointer to a pointer to a pointer, which pointer_type does not make.
  template<typename T>
  consteval auto run_time_pointer() -> bool {
    if constexpr (is_ptr<T>) {
      return !static_pointer<typename pointee<T>::type> && pointer_depth<T> < 3;
    } else {
      return false;
    }
  }

  template<typename T>
  auto rtype_of() -> const rtype* {
    if constexpr (run_time_pointer<T>()) {
      // Made on first use, and given its descriptor here.
      const auto* r = pointer_type(rtype_of<typename pointee<T>::type>());
      if constexpr (go_type<T>) {
        if (r->desc() == nullptr) {
          const type_descriptor* none = nullptr;
          r->descriptor.compare_exchange_strong(none, &descriptor<T>, std::memory_order_acq_rel);
        }
      }
      return r;
    } else {
      static const rtype_holder<T> holder;
      return &holder.r;
    }
  }

} // namespace detail

} // namespace eo
