// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/defined.h>
#include <eo/struct.h>

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

// Go interfaces by type erasure. A translated type lists its methods in a member alias _eo_methods,
// as Go's runtime type information does, and an interface value holds its dynamic value with a
// descriptor of the dynamic type and a table of the interface's methods. Satisfaction stays
// implicit: the table for a conversion is built at compile time from the method names, and for an
// assertion or a conversion between interfaces at run time.

namespace eo {

template<typename T>
class ptr;

template<typename T>
class slice;

template<typename K, typename V>
class map;

// A method name, as a template argument.
template<std::size_t N>
struct method_name {
  char chars[N]{};

  consteval method_name(const char (&s)[N]) {
    std::copy_n(s, N, chars);
  }

  constexpr auto view() const -> std::string_view {
    return {chars, N - 1};
  }
};

// One method of a translated type, F being its static member function, whose first parameter is
// the receiver, or one method of a translated interface, F being its member function.
template<auto F, method_name Name>
struct method {
  static constexpr auto fn = F;
  static constexpr auto name = Name.view();
};

// The methods of a translated type or interface, in declaration order.
template<typename... M>
struct methods {};

namespace detail {

  // The Go type name of a type, as the runtime and %T print it.
  template<typename T>
  auto type_name() -> std::string;

  // Go's fmt printer state, and the printing of one operand with a verb, from <eo/fmt.h>. `depth` is
  // 0 for an operand of Println or Printf, which prints a pointer to a struct, slice, or map as &
  // followed by its target.
  struct printer;

  template<typename T>
  void print_arg(printer& p, const T& value, int32_t verb, int depth);

  // Go's runtime panics, from <eo/panic.h> and <eo/ptr.h>.
  [[noreturn]] inline void panic_runtime_error(std::string message);
  [[noreturn]] inline void panic_nil_dereference();
  [[noreturn]] inline void panic_type_assertion(std::string message);

  template<typename T>
  inline constexpr bool is_ptr = false;

  template<typename T>
  inline constexpr bool is_ptr<ptr<T>> = true;

  template<typename T>
  struct pointee {
    using type = T;
  };

  template<typename T>
  struct pointee<ptr<T>> {
    using type = T;
  };

  template<typename T>
  concept has_methods = requires { typename T::_eo_methods; };

  template<typename T>
  inline constexpr bool is_slice = false;

  template<typename T>
  inline constexpr bool is_slice<slice<T>> = true;

  template<typename T>
  inline constexpr bool is_map = false;

  template<typename K, typename V>
  inline constexpr bool is_map<map<K, V>> = true;

  // Whether T is a translated Go type, which has a Go type name and so can be held in an interface:
  // a basic type, a string, struct{}, a type that declares _eo_name, such as a translated struct or
  // defined type, or a pointer, slice, or map of such types.
  template<typename T>
  consteval auto is_go_type() -> bool {
    if constexpr (requires { T::_eo_name; }) {
      return true;
    } else if constexpr (std::is_same_v<T, bool> || std::is_same_v<T, int8_t> || std::is_same_v<T, int16_t> ||
      std::is_same_v<T, int32_t> || std::is_same_v<T, int64_t> || std::is_same_v<T, uint8_t> ||
      std::is_same_v<T, uint16_t> || std::is_same_v<T, uint32_t> || std::is_same_v<T, uint64_t> ||
      std::is_same_v<T, float> || std::is_same_v<T, double> || std::is_same_v<T, std::string> ||
      std::is_same_v<T, empty_struct>) {
      return true;
    } else if constexpr (is_ptr<T>) {
      return is_go_type<typename pointee<T>::type>();
    } else if constexpr (is_slice<T>) {
      return is_go_type<typename T::value_type>();
    } else if constexpr (is_map<T>) {
      return is_go_type<typename T::key_type>() && is_go_type<typename T::mapped_type>();
    } else {
      return false;
    }
  }

  template<typename T>
  concept go_type = is_go_type<T>();

  template<typename List>
  inline constexpr std::size_t method_count = 0;

  template<typename... M>
  inline constexpr std::size_t method_count<methods<M...>> = sizeof...(M);

  template<typename F>
  struct method_traits;

  template<typename R, typename Receiver, typename... A>
  struct method_traits<R (*)(Receiver, A...)> {
    using receiver = Receiver;
    using signature = R(A...);
  };

  template<typename R, typename I, typename... A>
  struct method_traits<R (I::*)(A...) const> {
    using signature = R(A...);
  };

  template<typename M>
  using signature_of = typename method_traits<std::remove_cv_t<decltype(M::fn)>>::signature;

  // One object per signature, whose address identifies it across translation units.
  template<typename Signature>
  inline constexpr char signature_tag = 0;

  // A method of a dynamic type: its name and signature, and, in entry<Signature>, a function that
  // calls it on a value of that type.
  struct entry_base {
    std::string_view name;
    const void* signature;
  };

  template<typename Signature>
  struct entry;

  template<typename R, typename... A>
  struct entry<R(A...)> : entry_base {
    R (*fn)(const void*, A...);
  };

  // Calls F on the dynamic value at p, of type D. A value-receiver method of a pointer type
  // dereferences the pointer, which panics for nil, as Go's wrapper method does.
  template<typename D, auto F, typename Fn = std::remove_cv_t<decltype(F)>>
  struct thunk;

  template<typename D, auto F, typename R, typename Receiver, typename... A>
  struct thunk<D, F, R (*)(Receiver, A...)> {
    static auto call(const void* p, A... args) -> R {
      const auto& value = *static_cast<const D*>(p);
      if constexpr (std::is_same_v<Receiver, D>) {
        return F(value, std::move(args)...);
      } else {
        return F(*value, std::move(args)...);
      }
    }
  };

  // Go's method sets: a type T has the methods with a T receiver, and *T also those with a *T
  // receiver.
  template<typename D, typename M>
  consteval auto in_method_set() -> bool {
    using receiver = typename method_traits<std::remove_cv_t<decltype(M::fn)>>::receiver;
    if constexpr (is_ptr<D>) {
      return std::is_same_v<receiver, D> || std::is_same_v<receiver, typename pointee<D>::type>;
    } else {
      return std::is_same_v<receiver, D>;
    }
  }

  template<typename D, typename M>
  inline constexpr entry<signature_of<M>> entry_for{{M::name, &signature_tag<signature_of<M>>}, &thunk<D, M::fn>::call};

  template<typename D, typename List>
  struct method_set_of {
    static constexpr std::array<const entry_base*, 0> entries{};
  };

  template<typename D, typename... M>
  struct method_set_of<D, methods<M...>> {
    static constexpr std::size_t count = (std::size_t{0} + ... + (in_method_set<D, M>() ? 1 : 0));

    static constexpr auto entries = [] {
      auto result = std::array<const entry_base*, count>{};
      auto i = std::size_t{0};
      [[maybe_unused]] auto add = [&]<typename Method>() {
        if constexpr (in_method_set<D, Method>()) {
          result[i++] = &entry_for<D, Method>;
        }
      };
      (add.template operator()<M>(), ...);
      return result;
    }();
  };

  template<typename D>
  struct methods_of {
    using type = methods<>;
  };

  struct interface_base;

  // A pointer to an interface has no methods in Go.
  template<typename D>
    requires has_methods<typename pointee<D>::type> && (!std::is_base_of_v<interface_base, typename pointee<D>::type>)
  struct methods_of<D> {
    using type = typename pointee<D>::type::_eo_methods;
  };

  // The method set of a dynamic type D: the methods of D, or of T for D = ptr<T>, that are in it.
  template<typename D>
  using method_set = method_set_of<D, typename methods_of<D>::type>;

  inline constexpr auto no_method = static_cast<std::size_t>(-1);

  template<typename D, typename Signature, typename... M>
  consteval auto method_position_in(std::string_view name, methods<M...>) -> std::size_t {
    auto position = std::size_t{0};
    auto result = no_method;
    auto done = false;
    [[maybe_unused]] auto check = [&]<typename Method>() {
      if constexpr (in_method_set<D, Method>()) {
        if (!done && Method::name == name) {
          done = true;
          if constexpr (std::is_same_v<signature_of<Method>, Signature>) {
            result = position;
          }
        }
        ++position;
      }
    };
    (check.template operator()<M>(), ...);
    return result;
  }

  // The position in method_set<D>::entries of the method named `name`, or no_method if D's method
  // set has no such method with that signature. It compares names and types, not addresses, which
  // GCC does not fold in constant expressions under sanitizers.
  template<typename D, typename Signature>
  consteval auto method_position(std::string_view name) -> std::size_t {
    return method_position_in<D, Signature>(name, typename methods_of<D>::type{});
  }

  // The descriptor of a dynamic type: its Go name, its equality, which is null for an uncomparable
  // type, its printing with a verb, and its method set. Its address identifies the type.
  struct type_descriptor {
    auto (*name)() -> std::string;
    auto (*equal)(const void*, const void*) -> bool;
    void (*print)(printer& p, const void* value, int32_t verb, int depth);
    std::span<const entry_base* const> methods;
  };

  // Go's comparable types.
  template<typename T>
  inline constexpr bool go_comparable = std::equality_comparable<T>;

  template<typename D>
  auto equal_values(const void* a, const void* b) -> bool {
    return *static_cast<const D*>(a) == *static_cast<const D*>(b);
  }

  template<typename D>
  void print_dynamic(printer& p, const void* value, int32_t verb, int depth) {
    print_arg(p, *static_cast<const D*>(value), verb, depth);
  }

  template<typename D>
  constexpr auto equality_of() -> bool (*)(const void*, const void*) {
    if constexpr (go_comparable<D>) {
      return &equal_values<D>;
    } else {
      return nullptr;
    }
  }

  template<typename D>
  inline constexpr type_descriptor descriptor{
    &type_name<D>,
    equality_of<D>(),
    &print_dynamic<D>,
    method_set<D>::entries,
  };

  template<typename T>
  concept interface_type = std::is_base_of_v<interface_base, T>;

  // The table of interface I's methods for dynamic type D, built at compile time. A type that lacks
  // a method of I does not compile.
  template<typename I, typename D, typename List = typename I::_eo_methods>
  struct static_table;

  template<typename I, typename D, typename... M>
  struct static_table<I, D, methods<M...>> {
    static_assert((... && (method_position<D, signature_of<M>>(M::name) != no_method)),
      "the type does not have every method of the interface");
    static constexpr std::array<const entry_base*, sizeof...(M)> value{
      method_set<D>::entries[method_position<D, signature_of<M>>(M::name)]...};
  };

  // The position of member function M in an interface's method list.
  template<auto M, typename... Ms>
  consteval auto method_index(methods<Ms...>) -> std::size_t {
    auto i = std::size_t{0};
    auto found = sizeof...(Ms);
    [[maybe_unused]] auto check = [&]<typename Method>() {
      if constexpr (std::is_same_v<decltype(M), std::remove_cv_t<decltype(Method::fn)>>) {
        if (Method::fn == M) {
          found = i;
        }
      }
      ++i;
    };
    (check.template operator()<Ms>(), ...);
    return found;
  }

  // The interface value's representation, shared by every interface type.
  struct interface_base {
    const type_descriptor* _eo_type = nullptr;
    std::shared_ptr<const void> _eo_value;
    const entry_base* const* _eo_table = nullptr;

    // Go's ==: two nil interfaces are equal, and two non-nil ones when their dynamic types are
    // identical and their values are equal. Comparing two values of an uncomparable type panics.
    friend auto operator==(const interface_base& a, const interface_base& b) -> bool {
      if (a._eo_type != b._eo_type) {
        return false;
      }
      if (a._eo_type == nullptr) {
        return true;
      }
      if (a._eo_type->equal == nullptr) {
        panic_runtime_error("comparing uncomparable type " + a._eo_type->name());
      }
      return a._eo_type->equal(a._eo_value.get(), b._eo_value.get());
    }

    friend auto operator==(const interface_base& a, std::nullptr_t) noexcept -> bool {
      return a._eo_type == nullptr;
    }
  };

  // Whether interface J has every method of interface I, as Go requires of an assignment of a J to
  // an I.
  template<typename Signature, typename... N>
  consteval auto lists_method(std::string_view name, methods<N...>) -> bool {
    return (... || (N::name == name && std::is_same_v<signature_of<N>, Signature>));
  }

  template<typename J, typename... M>
  consteval auto has_all_methods(methods<M...>) -> bool {
    return (... && lists_method<signature_of<M>>(M::name, typename J::_eo_methods{}));
  }

  // The methods of interface I, ordered by name, as Go reports the first missing one.
  template<typename I, typename List = typename I::_eo_methods>
  struct interface_methods;

  template<typename I, typename... M>
  struct interface_methods<I, methods<M...>> {
    struct named {
      std::string_view name;
      const void* signature;
      std::size_t index;
    };

    static auto sorted() -> std::vector<named> {
      [[maybe_unused]] auto i = std::size_t{0};
      auto result = std::vector<named>{named{M::name, &signature_tag<signature_of<M>>, i++}...};
      std::ranges::sort(result, {}, &named::name);
      return result;
    }
  };

  // The table of interface I's methods for a dynamic type known only at run time, or the name of
  // the first method it lacks. Tables are built once per pair and kept, as Go's itab cache does.
  template<typename I>
  auto dynamic_table(const type_descriptor* type, std::string_view* missing) -> const std::vector<const entry_base*>* {
    if constexpr (method_count<typename I::_eo_methods> == 0) {
      // Every type has the methods of an empty interface, such as any.
      static const auto& none = *new std::vector<const entry_base*>;
      return &none;
    }
    // Never destroyed, so a thread that still runs during static destruction can use them.
    static auto& mutex = *new std::shared_mutex;
    static auto& tables = *new std::map<const type_descriptor*, std::vector<const entry_base*>>;
    static const auto& wanted = *new std::vector(interface_methods<I>::sorted());
    {
      auto lock = std::shared_lock{mutex};
      if (auto it = tables.find(type); it != tables.end()) {
        return &it->second;
      }
    }
    auto table = std::vector<const entry_base*>(wanted.size());
    for (const auto& w : wanted) {
      auto found = std::ranges::find(type->methods, w.name, &entry_base::name);
      if (found == type->methods.end() || (*found)->signature != w.signature) {
        *missing = w.name;
        return nullptr;
      }
      table[w.index] = *found;
    }
    auto lock = std::unique_lock{mutex};
    return &tables.try_emplace(type, std::move(table)).first->second;
  }

} // namespace detail

// The base of a translated interface I. An interface value is nil or holds a dynamic value, which
// is shared and never modified, so copying the interface value behaves like Go's copy.
template<typename I>
class interface : public detail::interface_base {
public:
  constexpr interface() noexcept = default;
  constexpr interface(std::nullptr_t) noexcept {}

  // A value of a non-interface type that has every method of I.
  template<detail::go_type T>
    requires(!detail::interface_type<T>)
  interface(T value) {
    _eo_type = &detail::descriptor<T>;
    _eo_value = std::make_shared<const T>(std::move(value));
    _eo_table = detail::static_table<I, T>::value.data();
  }

  // An interface value whose interface has every method of I. Go checks this at compile time.
  template<detail::interface_type J>
    requires(!std::is_base_of_v<interface, J>)
  interface(const J& other) {
    static_assert(detail::has_all_methods<J>(typename I::_eo_methods{}),
      "the interface does not have every method of the interface it is converted to");
    if (other._eo_type == nullptr) {
      return;
    }
    auto missing = std::string_view{};
    auto table = detail::dynamic_table<I>(other._eo_type, &missing);
    if (table == nullptr) {
      detail::panic_type_assertion("interface conversion: " + other._eo_type->name() + " is not " +
        std::string{I::_eo_name} + ": missing method " + std::string{missing});
    }
    _eo_type = other._eo_type;
    _eo_value = other._eo_value;
    _eo_table = table->data();
  }

protected:
  // Calls method M of I, a pointer to I's member function. Calling a method of a nil interface
  // panics.
  template<auto M, typename... A>
  auto _eo_call(A&&... args) const -> decltype(auto) {
    using signature = typename detail::method_traits<decltype(M)>::signature;
    if (_eo_type == nullptr) {
      detail::panic_nil_dereference();
    }
    auto e = static_cast<const detail::entry<signature>*>(_eo_table[index_of<M>()]);
    return e->fn(_eo_value.get(), std::forward<A>(args)...);
  }

private:
  template<auto M>
  static consteval auto index_of() -> std::size_t {
    constexpr auto i = detail::method_index<M>(typename I::_eo_methods{});
    static_assert(i < detail::method_count<typename I::_eo_methods>, "not a method of the interface");
    return i;
  }
};

// Go's any and interface{}.
struct any : interface<any> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "interface {}";
  using _eo_methods = methods<>;
};

// Go's predeclared error interface.
struct error : interface<error> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "error";

  auto Error() const -> std::string;

  using _eo_methods = methods<method<&error::Error, "Error">>;
};

inline auto error::Error() const -> std::string {
  return _eo_call<&error::Error>();
}

namespace detail {

  template<typename I>
  auto interface_name() -> std::string {
    return std::string{I::_eo_name};
  }

  // x.(T) and v, ok := x.(T).
  template<typename T>
  struct assert_fn {
    template<interface_type I>
    auto operator()(const I& x) const -> T {
      auto [value, ok] = try_assert(x);
      if (!ok) {
        fail(x);
      }
      return value;
    }

    template<interface_type I>
    static auto try_assert(const I& x) -> std::pair<T, bool> {
      if constexpr (interface_type<T>) {
        if (x._eo_type == nullptr) {
          return {T{}, false};
        }
        auto missing = std::string_view{};
        auto table = dynamic_table<T>(x._eo_type, &missing);
        if (table == nullptr) {
          return {T{}, false};
        }
        auto result = T{};
        result._eo_type = x._eo_type;
        result._eo_value = x._eo_value;
        result._eo_table = table->data();
        return {std::move(result), true};
      } else {
        if (x._eo_type != &descriptor<T>) {
          return {T{}, false};
        }
        return {*static_cast<const T*>(x._eo_value.get()), true};
      }
    }

    template<interface_type I>
    [[noreturn]] static void fail(const I& x) {
      if constexpr (interface_type<T>) {
        if (x._eo_type == nullptr) {
          panic_type_assertion("interface conversion: interface is nil, not " + interface_name<T>());
        }
        auto missing = std::string_view{};
        dynamic_table<T>(x._eo_type, &missing);
        panic_type_assertion("interface conversion: " + x._eo_type->name() + " is not " + interface_name<T>() +
          ": missing method " + std::string{missing});
      } else {
        auto dynamic = x._eo_type == nullptr ? std::string{"nil"} : x._eo_type->name();
        panic_type_assertion(
          "interface conversion: " + interface_name<I>() + " is " + dynamic + ", not " + type_name<T>());
      }
    }
  };

  template<typename T>
  struct assert_ok_fn {
    template<interface_type I>
    auto operator()(const I& x) const -> std::pair<T, bool> {
      return assert_fn<T>::try_assert(x);
    }
  };

  // Whether x holds a T, for a type switch case.
  template<typename T>
  struct holds_fn {
    template<interface_type I>
    auto operator()(const I& x) const -> bool {
      if constexpr (interface_type<T>) {
        auto missing = std::string_view{};
        return x._eo_type != nullptr && dynamic_table<T>(x._eo_type, &missing) != nullptr;
      } else {
        return x._eo_type == &descriptor<T>;
      }
    }
  };

  template<typename T>
  auto type_name() -> std::string {
    using U = std::remove_cv_t<T>;
    if constexpr (requires { U::_eo_name; }) {
      return std::string{U::_eo_name};
    } else if constexpr (std::is_same_v<U, bool>) {
      return "bool";
    } else if constexpr (std::is_same_v<U, int8_t>) {
      return "int8";
    } else if constexpr (std::is_same_v<U, int16_t>) {
      return "int16";
    } else if constexpr (std::is_same_v<U, int32_t>) {
      return "int32";
    } else if constexpr (std::is_same_v<U, int64_t>) {
      // Go's int and int64 are both int64_t, which prints as int.
      return "int";
    } else if constexpr (std::is_same_v<U, uint8_t>) {
      return "uint8";
    } else if constexpr (std::is_same_v<U, uint16_t>) {
      return "uint16";
    } else if constexpr (std::is_same_v<U, uint32_t>) {
      return "uint32";
    } else if constexpr (std::is_same_v<U, uint64_t>) {
      return "uint";
    } else if constexpr (std::is_same_v<U, float>) {
      return "float32";
    } else if constexpr (std::is_same_v<U, double>) {
      return "float64";
    } else if constexpr (std::is_same_v<U, std::string>) {
      return "string";
    } else if constexpr (std::is_same_v<U, empty_struct>) {
      return "struct {}";
    } else if constexpr (is_ptr<U>) {
      return "*" + type_name<typename pointee<U>::type>();
    } else if constexpr (is_slice<U>) {
      return "[]" + type_name<typename U::value_type>();
    } else if constexpr (is_map<U>) {
      return "map[" + type_name<typename U::key_type>() + "]" + type_name<typename U::mapped_type>();
    } else {
      static_assert(sizeof(U) == 0, "a type held in an interface needs a Go type name");
    }
  }

} // namespace detail

// Function objects, called qualified, like the string helpers (see <eo/string.h>).
template<typename T>
inline constexpr detail::assert_fn<T> type_assert{};

template<typename T>
inline constexpr detail::assert_ok_fn<T> type_assert_ok{};

template<typename T>
inline constexpr detail::holds_fn<T> holds{};

} // namespace eo

// The definitions of the panics declared above, and of print_arg.
#include <eo/panic.h>
