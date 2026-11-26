// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/array.h>
#include <eo/defined.h>
#include <eo/fields.h>
#include <eo/interface.h>
#include <eo/rtype.h>
#include <eo/struct.h>

#include <boost/pfr.hpp>

#include <cmath>
#include <cstddef>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>

// The map key operations of Go's comparable types: the hash, the test for a key that is unequal to
// itself, and the test for equal keys whose bits differ. <eo/map.h> keeps its tables by them, and an
// interface's type descriptor holds them for its dynamic type, so that an interface value can be a
// key. This header is included last from <eo/panic.h>, with <eo/rtype.h>, and reaches ptr and chan
// only through names that depend on a template parameter, for the same reason as <eo/rtype.h>.

namespace eo::detail {

// Whether a type can be a map key: a bool, a number, a string, a pointer, a channel, an interface,
// eo::empty_struct, a type that names its identity with _eo_key(), such as reflect.Type, or an array
// or struct of such types. These are Go's comparable types that Eo maps.
template<typename K>
consteval auto is_key_type() -> bool {
  if constexpr (defined_type<K>) {
    return is_key_type<typename K::_eo_underlying>();
  } else if constexpr (std::is_arithmetic_v<K> || std::is_same_v<K, std::string>) {
    return true;
  } else if constexpr (is_ptr<K> || is_chan_type<K> || std::is_base_of_v<interface_base, K> ||
    std::is_same_v<K, empty_struct>) {
    return true;
  } else if constexpr (requires(const K& k) { k._eo_key(); }) {
    return true;
  } else if constexpr (is_std_array<K>) {
    return is_key_type<typename K::value_type>();
  } else if constexpr (go_struct<K>) {
    return []<std::size_t... I>(std::index_sequence<I...>) {
      return (is_key_type<boost::pfr::tuple_element_t<I, K>>() && ...);
    }(std::make_index_sequence<boost::pfr::tuple_size_v<K>>{});
  } else {
    return false;
  }
}

// Whether a key of type K holds an interface, whose dynamic value may be of a type that cannot be a
// key.
template<typename K>
consteval auto has_interface_key() -> bool {
  if constexpr (defined_type<K>) {
    return has_interface_key<typename K::_eo_underlying>();
  } else if constexpr (std::is_base_of_v<interface_base, K>) {
    return true;
  } else if constexpr (is_std_array<K>) {
    return has_interface_key<typename K::value_type>();
  } else if constexpr (go_struct<K> && !std::is_same_v<K, empty_struct>) {
    return []<std::size_t... I>(std::index_sequence<I...>) {
      return (has_interface_key<boost::pfr::tuple_element_t<I, K>>() || ...);
    }(std::make_index_sequence<boost::pfr::tuple_size_v<K>>{});
  } else {
    return false;
  }
}

// The name of the first type in a key, in Go's order of its elements and fields, that cannot be a
// key, as the dynamic type of an interface in it can be, or empty if there is none.
template<typename K>
auto key_check(const K& k) -> std::string {
  if constexpr (!has_interface_key<K>()) {
    return {};
  } else if constexpr (defined_type<K>) {
    return key_check(underlying(k));
  } else if constexpr (std::is_base_of_v<interface_base, K>) {
    return k._eo_type == nullptr ? std::string{} : k._eo_type->key_check(k._eo_value.get());
  } else if constexpr (is_std_array<K>) {
    for (const auto& element : k) {
      if (auto name = key_check(element); !name.empty()) {
        return name;
      }
    }
    return {};
  } else {
    auto name = std::string{};
    boost::pfr::for_each_field(k, [&](const auto& field) {
      if (name.empty()) {
        name = key_check(field);
      }
    });
    return name;
  }
}

// Go's mapKeyError, which a lookup or a delete in a nil or empty map checks its key with, since it
// does not hash the key.
template<typename K>
void check_key_of_empty_map(const K& k) {
  if constexpr (has_interface_key<K>()) {
    if (auto name = key_check(k); !name.empty()) {
      throw Panic{any{runtime::unhashableTypeError{std::move(name)}}};
    }
  }
}

// Whether a key of type K can be unequal to itself, as a float NaN is, or a struct with a NaN field,
// or an interface that holds one.
template<typename K>
consteval auto has_float_key() -> bool {
  if constexpr (defined_type<K>) {
    return has_float_key<typename K::_eo_underlying>();
  } else if constexpr (std::is_floating_point_v<K> || std::is_base_of_v<interface_base, K>) {
    return true;
  } else if constexpr (is_std_array<K>) {
    return has_float_key<typename K::value_type>();
  } else if constexpr (go_struct<K> && !std::is_same_v<K, empty_struct>) {
    return []<std::size_t... I>(std::index_sequence<I...>) {
      return (has_float_key<boost::pfr::tuple_element_t<I, K>>() || ...);
    }(std::make_index_sequence<boost::pfr::tuple_size_v<K>>{});
  } else {
    return false;
  }
}

inline auto combine_hash(std::size_t h, std::size_t v) -> std::size_t {
  return h ^ (v + 0x9e3779b97f4a7c15 + (h << 6) + (h >> 2));
}

// Go's hash of a map key: equal keys hash alike. 0.0 and -0.0 are equal, so -0.0 hashes as 0.0,
// which the standard library's float hash does not promise. An interface hashes its dynamic type
// and its value, and panics as Go does when the value's type cannot be a key.
template<typename K>
auto hash_key(const K& k) -> std::size_t {
  if constexpr (defined_type<K>) {
    return hash_key(underlying(k));
  } else if constexpr (std::is_floating_point_v<K>) {
    return std::hash<K>{}(k == K{0} ? K{0} : k);
  } else if constexpr (std::is_arithmetic_v<K> || std::is_same_v<K, std::string>) {
    return std::hash<K>{}(k);
  } else if constexpr (is_ptr<K>) {
    return std::hash<const void*>{}(static_cast<const void*>(k.get()));
  } else if constexpr (is_chan_type<K>) {
    return std::hash<const void*>{}(static_cast<const void*>(k.shared_impl().get()));
  } else if constexpr (std::is_base_of_v<interface_base, K>) {
    if (k._eo_type == nullptr) {
      return 0;
    }
    return combine_hash(std::hash<const void*>{}(k._eo_type), k._eo_type->key_hash(k._eo_value.get()));
  } else if constexpr (std::is_same_v<K, empty_struct>) {
    return 0;
  } else if constexpr (requires { k._eo_key(); }) {
    return std::hash<decltype(k._eo_key())>{}(k._eo_key());
  } else if constexpr (is_std_array<K>) {
    auto h = std::size_t{0};
    for (const auto& element : k) {
      h = combine_hash(h, hash_key(element));
    }
    return h;
  } else if constexpr (go_struct<K>) {
    auto h = std::size_t{0};
    boost::pfr::for_each_field(k, [&](const auto& field) { h = combine_hash(h, hash_key(field)); });
    return h;
  } else {
    static_assert(sizeof(K) == 0,
      "a map key must be a bool, number, string, pointer, channel, or interface, or an array or struct "
      "of such types; other key types are not covered yet");
  }
}

template<typename K>
struct key_hash {
  auto operator()(const K& k) const -> std::size_t {
    return hash_key(k);
  }
};

// Whether a key is unequal to itself, as NaN is. Such a key can be stored but never found. A key that
// holds a value whose type cannot be a key panics first, as Go's hash of it does, so that a lookup,
// an assignment, and a delete with such a key panic with Go's error.
template<typename K>
auto is_nan_key(const K& k) -> bool {
  if constexpr (has_interface_key<K>()) {
    if (auto name = key_check(k); !name.empty()) {
      panic_runtime_error("hash of unhashable type " + name);
    }
  }
  if constexpr (has_float_key<K>()) {
    return !(k == k);
  } else {
    return false;
  }
}

// Whether two equal keys differ in their bits, as 0.0 and -0.0 do. Go's map assignment stores the
// new key in that case.
template<typename K>
auto key_bits_differ(const K& a, const K& b) -> bool {
  if constexpr (defined_type<K>) {
    return key_bits_differ(underlying(a), underlying(b));
  } else if constexpr (std::is_floating_point_v<K>) {
    return std::signbit(a) != std::signbit(b);
  } else if constexpr (std::is_base_of_v<interface_base, K>) {
    return a._eo_type != nullptr && a._eo_type == b._eo_type &&
      a._eo_type->key_bits_differ(a._eo_value.get(), b._eo_value.get());
  } else if constexpr (is_std_array<K> && has_float_key<K>()) {
    for (std::size_t i = 0; i < a.size(); ++i) {
      if (key_bits_differ(a[i], b[i])) {
        return true;
      }
    }
    return false;
  } else if constexpr (go_struct<K> && has_float_key<K>()) {
    return [&]<std::size_t... I>(std::index_sequence<I...>) {
      return (key_bits_differ(boost::pfr::get<I>(a), boost::pfr::get<I>(b)) || ...);
    }(std::make_index_sequence<boost::pfr::tuple_size_v<K>>{});
  } else {
    return false;
  }
}

template<typename K>
auto compare_keys(const K& a, const K& b) -> int;

// The key operations of a dynamic type D, which its type descriptor holds. A type that cannot be a
// key panics with Go's runtime error.
template<typename D>
[[noreturn]] void panic_unhashable() {
  panic_runtime_error("hash of unhashable type " + type_name<D>());
}

template<typename D>
auto key_hash_dynamic(const void* value) -> std::size_t {
  if constexpr (is_key_type<D>()) {
    return hash_key(*static_cast<const D*>(value));
  } else {
    panic_unhashable<D>();
  }
}

template<typename D>
auto key_check_dynamic(const void* value) -> std::string {
  if constexpr (is_key_type<D>()) {
    return key_check(*static_cast<const D*>(value));
  } else {
    return type_name<D>();
  }
}

template<typename D>
auto key_bits_differ_dynamic(const void* a, const void* b) -> bool {
  if constexpr (is_key_type<D>()) {
    return key_bits_differ(*static_cast<const D*>(a), *static_cast<const D*>(b));
  } else {
    return false;
  }
}

template<typename D>
auto key_compare_dynamic(const void* a, const void* b) -> int {
  if constexpr (is_key_type<D>()) {
    return compare_keys(*static_cast<const D*>(a), *static_cast<const D*>(b));
  } else {
    return 0;
  }
}

} // namespace eo::detail
