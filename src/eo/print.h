// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/float.h>
#include <eo/interface.h>
#include <eo/panic.h>
#include <eo/utf8.h>

#include <boost/pfr.hpp>
#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <functional>
#include <iterator>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace fmt {

// A time point, which the printing below reaches through its fallback to fmt's formatters. It is
// declared here, before the printing, so that every translation unit sees the same formatter.
template<typename Clock>
struct formatter<std::chrono::time_point<Clock>> {
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.end();
  }
  template<typename FormatContext>
  auto format(const std::chrono::time_point<Clock>& p, FormatContext& ctx) const {
    auto buffer = std::vector<char>(256);
    auto t = Clock::to_time_t(p);
    auto size = std::strftime(buffer.data(), buffer.size(), "%F %T %z %Z", std::localtime(&t));
    return fmt::format_to(ctx.out(), "{}", std::string_view(buffer.data(), size));
  }
};

} // namespace fmt

// Go's fmt printing. One engine, after Go's pp, prints every operand of Println, Sprint, Sprintf,
// Printf, Fprintf, and Errorf: print_arg follows Go's printArg, handle_methods its handleMethods, and
// print_value its printValue.

namespace eo::detail {

template<typename T>
inline constexpr bool is_std_array = false;

template<typename T, std::size_t N>
inline constexpr bool is_std_array<std::array<T, N>> = true;

// A translated Go struct: a class aggregate, which Boost.PFR can visit field by field. This also
// matches other aggregates, so it is ambiguous with fmt's own formatters for aggregate types, such as
// std::tm in <fmt/chrono.h>, std::monostate in <fmt/std.h>, and fmt::streamed. Eo uses none of them
// for Go values.
template<typename T>
concept go_struct = std::is_class_v<T> && std::is_aggregate_v<T> && !is_std_array<T>;

template<typename T>
consteval auto is_composite_type() -> bool {
  if constexpr (defined_type<T>) {
    return is_slice<typename T::_eo_underlying> || is_map<typename T::_eo_underlying>;
  } else {
    return go_struct<T> || is_slice<T> || is_map<T>;
  }
}

// Whether a pointer to T prints as & followed by its target: a struct, a slice, or a map, or a defined
// type whose underlying type is a slice or a map.
template<typename T>
inline constexpr bool is_composite = is_composite_type<T>();

// Go's internal/fmtsort order of map keys, which fmt prints a map in: numbers by value, with NaN
// before every other float, strings by bytes, false before true, pointers by address, and structs
// field by field. Returns a negative number, zero, or a positive number.
template<typename K>
auto compare_keys(const K& a, const K& b) -> int {
  if constexpr (defined_type<K>) {
    return compare_keys(underlying(a), underlying(b));
  } else if constexpr (std::is_floating_point_v<K>) {
    if (std::isnan(a) || std::isnan(b)) {
      return std::isnan(a) ? (std::isnan(b) ? 0 : -1) : 1;
    }
    return a < b ? -1 : (b < a ? 1 : 0);
  } else if constexpr (std::is_arithmetic_v<K>) {
    return a < b ? -1 : (b < a ? 1 : 0);
  } else if constexpr (std::is_same_v<K, std::string>) {
    auto c = a.compare(b);
    return c < 0 ? -1 : (c > 0 ? 1 : 0);
  } else if constexpr (is_ptr<K>) {
    return std::less<const void*>{}(a.get(), b.get()) ? -1 : (a.get() == b.get() ? 0 : 1);
  } else if constexpr (std::is_same_v<K, empty_struct>) {
    return 0;
  } else {
    auto c = 0;
    [&]<std::size_t... I>(std::index_sequence<I...>) {
      ((c = c != 0 ? c : compare_keys(boost::pfr::get<I>(a), boost::pfr::get<I>(b))), ...);
    }(std::make_index_sequence<boost::pfr::tuple_size_v<K>>{});
    return c;
  }
}

// The state of one call that prints, as Go's pp.
struct printer {
  std::string buf;
  // Errorf, where %w prints an error operand with %v.
  bool wrap_errs = false;
  // Printing the operand of a bad verb, which skips Error and String methods, as Go does.
  bool erroring = false;
  // Printing the value of a panic in an Error or String method. Another panic then propagates, as Go's
  // catchPanic does.
  bool panicking = false;
};

template<typename T>
inline constexpr bool is_string_like = std::is_convertible_v<const T&, std::string_view>;

// The Go type name of an operand, as %T and bad verbs print it: an interface value's dynamic type.
template<typename T>
auto type_of(const T& value) -> std::string {
  if constexpr (interface_type<T>) {
    return value._eo_type == nullptr ? std::string{"<nil>"} : value._eo_type->name();
  } else if constexpr (std::is_same_v<T, std::nullptr_t>) {
    return "<nil>";
  } else if constexpr (go_type<T>) {
    return type_name<T>();
  } else if constexpr (is_string_like<T>) {
    return "string";
  } else {
    return "?";
  }
}

// %!verb(type=value), with the value printed as %v without its methods, or %!verb(<nil>).
template<typename T>
void bad_verb(printer& p, int32_t verb, const T& value) {
  auto erroring = std::exchange(p.erroring, true);
  p.buf += "%!";
  append_rune(p.buf, verb);
  p.buf += '(';
  if constexpr (interface_type<T>) {
    if (value == nullptr) {
      p.buf += "<nil>";
    } else {
      p.buf += type_of(value);
      p.buf += '=';
      print_arg(p, value, 'v', 0);
    }
  } else {
    p.buf += type_of(value);
    p.buf += '=';
    print_arg(p, value, 'v', 0);
  }
  p.buf += ')';
  p.erroring = erroring;
}

inline void append_hex(std::string& out, unsigned char byte, bool upper) {
  const char* digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
  out += digits[byte >> 4];
  out += digits[byte & 0xf];
}

// Go's unicode.IsPrint for a code point other than ASCII, except that it treats unassigned code
// points as printable. It rejects C1 controls, spaces other than U+0020, the common format
// characters, private use, and noncharacters.
inline auto is_print(char32_t r) -> bool {
  if (r < 0xa1) {
    return false;
  }
  return !(r == 0xad || r == 0x1680 || (r >= 0x2000 && r <= 0x200f) || (r >= 0x2028 && r <= 0x202f) ||
    (r >= 0x205f && r <= 0x2064) || r == 0x3000 || (r >= 0xe000 && r <= 0xf8ff) || r == 0xfeff ||
    (r >= 0xfff9 && r <= 0xfffb) || r == 0xfffe || r == 0xffff || (r >= 0xf0000));
}

// Go's strconv.appendEscapedRune.
inline void append_escaped_rune(std::string& out, char32_t r, char quote) {
  if (r == static_cast<char32_t>(quote) || r == '\\') {
    out += '\\';
    out += static_cast<char>(r);
    return;
  }
  if ((r >= 0x20 && r < 0x7f) || (r >= 0x80 && r <= 0x10ffff && !(r >= 0xd800 && r <= 0xdfff) && is_print(r))) {
    append_rune(out, static_cast<int32_t>(r));
    return;
  }
  switch (r) {
  case '\a':
    out += "\\a";
    return;
  case '\b':
    out += "\\b";
    return;
  case '\f':
    out += "\\f";
    return;
  case '\n':
    out += "\\n";
    return;
  case '\r':
    out += "\\r";
    return;
  case '\t':
    out += "\\t";
    return;
  case '\v':
    out += "\\v";
    return;
  default:
    break;
  }
  if (r < 0x20 || r == 0x7f) {
    out += "\\x";
    append_hex(out, static_cast<unsigned char>(r), false);
    return;
  }
  if (r > 0x10ffff || (r >= 0xd800 && r <= 0xdfff)) {
    r = 0xfffd;
  }
  if (r < 0x10000) {
    fmt::format_to(std::back_inserter(out), "\\u{:04x}", static_cast<uint32_t>(r));
  } else {
    fmt::format_to(std::back_inserter(out), "\\U{:08x}", static_cast<uint32_t>(r));
  }
}

// Go's strconv.Quote: a byte that does not start a valid UTF-8 sequence is \x followed by its hex.
inline void append_quoted(std::string& out, const std::string& s) {
  out += '"';
  for (std::size_t i = 0; i < s.size();) {
    auto [r, n] = decode_rune(s, i);
    if (n == 1 && r == 0xfffd) {
      out += "\\x";
      append_hex(out, static_cast<unsigned char>(s[i]), false);
    } else {
      append_escaped_rune(out, r, '"');
    }
    i += n;
  }
  out += '"';
}

// Go's fmtString and fmtBytes for the verbs of a string: false for another verb.
inline auto print_string(printer& p, const std::string& s, int32_t verb) -> bool {
  switch (verb) {
  case 'v':
  case 's':
    p.buf += s;
    return true;
  case 'x':
  case 'X':
    for (auto c : s) {
      append_hex(p.buf, static_cast<unsigned char>(c), verb == 'X');
    }
    return true;
  case 'q':
    append_quoted(p.buf, s);
    return true;
  default:
    return false;
  }
}

template<go_integer I>
void print_integer(printer& p, I v, int32_t verb) {
  auto out = std::back_inserter(p.buf);
  switch (verb) {
  case 'v':
  case 'd':
    fmt::format_to(out, "{}", v);
    return;
  case 'b':
    fmt::format_to(out, "{:b}", v);
    return;
  case 'o':
    fmt::format_to(out, "{:o}", v);
    return;
  case 'O':
    // The sign, then the prefix 0o, then the octal digits of the magnitude.
    if (std::cmp_less(v, 0)) {
      fmt::format_to(out, "-0o{:o}", static_cast<uint64_t>(0) - static_cast<uint64_t>(v));
    } else {
      fmt::format_to(out, "0o{:o}", v);
    }
    return;
  case 'x':
    fmt::format_to(out, "{:x}", v);
    return;
  case 'X':
    fmt::format_to(out, "{:X}", v);
    return;
  case 'c':
    append_rune(p.buf, v);
    return;
  case 'q': {
    // Go's %q of an integer is its quoted rune, with U+FFFD for an invalid code point.
    auto r = char32_t{0xfffd};
    if (std::cmp_greater_equal(v, 0) && std::cmp_less_equal(v, 0x10ffff) &&
      !(std::cmp_greater_equal(v, 0xd800) && std::cmp_less_equal(v, 0xdfff))) {
      r = static_cast<char32_t>(v);
    }
    p.buf += '\'';
    append_escaped_rune(p.buf, r, '\'');
    p.buf += '\'';
    return;
  }
  case 'U':
    // Go formats the bits of the value widened to 64 bits, so a negative value is sign-extended.
    if constexpr (std::is_signed_v<I>) {
      fmt::format_to(out, "U+{:04X}", static_cast<uint64_t>(static_cast<int64_t>(v)));
    } else {
      fmt::format_to(out, "U+{:04X}", static_cast<uint64_t>(v));
    }
    return;
  default:
    bad_verb(p, verb, v);
  }
}

template<std::floating_point F>
void print_float(printer& p, F v, int32_t verb) {
  auto out = std::back_inserter(p.buf);
  switch (verb) {
  case 'v':
  case 'g':
    fmt::format_to(out, "{}", go_value(v));
    return;
  case 'e':
  case 'E':
  case 'f':
  case 'F':
    if (std::isnan(v)) {
      p.buf += "NaN";
    } else if (std::isinf(v)) {
      p.buf += v > 0 ? "+Inf" : "-Inf";
    } else if (verb == 'e') {
      fmt::format_to(out, "{:e}", v);
    } else if (verb == 'E') {
      fmt::format_to(out, "{:E}", v);
    } else {
      fmt::format_to(out, "{:f}", v);
    }
    return;
  default:
    bad_verb(p, verb, v);
  }
}

// The method named `name` with signature `string()` in T's method set. It must exist.
template<typename T>
auto string_method(std::size_t position) -> const entry<std::string()>* {
  return static_cast<const entry<std::string()>*>(method_set<T>::entries[position]);
}

// Calls an Error or String method for fmt and prints its result with the verb. A panic in it prints
// as Go's fmt prints it, and as <nil> for a nil pointer receiver.
template<typename T>
void print_method(printer& p, const T& value, const entry<std::string()>* method, int32_t verb) {
  auto s = std::string{};
  try {
    s = method->fn(&value);
  } catch (...) {
    if constexpr (is_ptr<T>) {
      if (value == nullptr) {
        p.buf += "<nil>";
        return;
      }
    }
    if (p.panicking) {
      throw;
    }
    // Any exception is a panic, whose value prints as recover() would return it.
    auto recovered = panic_value(std::current_exception());
    p.buf += "%!";
    append_rune(p.buf, verb);
    p.buf += "(PANIC=";
    p.buf += method->name;
    p.buf += " method: ";
    p.panicking = true;
    print_arg(p, recovered, 'v', 0);
    p.panicking = false;
    p.buf += ')';
    return;
  }
  print_string(p, s, verb);
}

// Go's handleMethods: %w of an error in Errorf, and Error or String, which Go prefers in that order,
// for %v, %s, %x, %X, and %q. Returns whether it printed the operand.
template<typename T>
auto handle_methods(printer& p, const T& value, int32_t verb) -> bool {
  if (p.erroring) {
    return false;
  }
  constexpr auto error = method_position<T, std::string()>("Error");
  constexpr auto string = method_position<T, std::string()>("String");
  if (verb == 'w') {
    if (error == no_method || !p.wrap_errs) {
      bad_verb(p, verb, value);
      return true;
    }
    verb = 'v';
  }
  if constexpr (error != no_method || string != no_method) {
    switch (verb) {
    case 'v':
    case 's':
    case 'x':
    case 'X':
    case 'q': {
      constexpr auto position = error != no_method ? error : string;
      print_method(p, value, string_method<T>(position), verb);
      return true;
    }
    default:
      break;
    }
  }
  return false;
}

// Go's fmtPointer.
template<typename T>
void print_pointer(printer& p, const T& value, int32_t verb) {
  auto address = reinterpret_cast<std::uintptr_t>(value.get());
  switch (verb) {
  case 'v':
    if (address == 0) {
      p.buf += "<nil>";
    } else {
      fmt::format_to(std::back_inserter(p.buf), "0x{:x}", address);
    }
    return;
  case 'b':
  case 'o':
  case 'd':
  case 'x':
  case 'X':
    print_integer(p, static_cast<uint64_t>(address), verb);
    return;
  default:
    bad_verb(p, verb, value);
  }
}

// Go's printValue, after the methods: the operand's kind decides how it prints.
template<typename T>
void print_value(printer& p, const T& value, int32_t verb, int depth) {
  if constexpr (is_slice<T>) {
    if constexpr (std::is_same_v<typename T::value_type, uint8_t>) {
      if (verb == 's' || verb == 'q' || verb == 'x' || verb == 'X') {
        print_string(p, std::string(value.begin(), value.end()), verb);
        return;
      }
    }
    p.buf += '[';
    auto first = true;
    for (const auto& element : value) {
      if (!first) {
        p.buf += ' ';
      }
      print_arg(p, element, verb, depth + 1);
      first = false;
    }
    p.buf += ']';
  } else if constexpr (is_map<T>) {
    using K = typename T::key_type;
    using V = typename T::mapped_type;
    auto entries = std::vector<std::pair<const K*, const V*>>{};
    if (auto* t = value.table(); t != nullptr) {
      entries.reserve(static_cast<std::size_t>(t->size()));
      t->for_each([&](const K& k, const V& v) { entries.emplace_back(&k, &v); });
    }
    std::stable_sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
      return compare_keys(*a.first, *b.first) < 0;
    });
    p.buf += "map[";
    auto first = true;
    for (const auto& [k, v] : entries) {
      if (!first) {
        p.buf += ' ';
      }
      print_arg(p, *k, verb, depth + 1);
      p.buf += ':';
      print_arg(p, *v, verb, depth + 1);
      first = false;
    }
    p.buf += ']';
  } else if constexpr (is_ptr<T>) {
    // A pointer operand to a struct, slice, or map prints as & followed by its target, and any other
    // pointer as <nil> or its address.
    if constexpr (is_composite<typename pointee<T>::type>) {
      if (depth == 0 && value != nullptr) {
        p.buf += '&';
        print_arg(p, *value, verb, depth + 1);
        return;
      }
    }
    print_pointer(p, value, verb);
  } else if constexpr (go_struct<T>) {
    p.buf += '{';
    auto first = true;
    boost::pfr::for_each_field(value, [&](const auto& field) {
      if (!first) {
        p.buf += ' ';
      }
      print_arg(p, field, verb, depth + 1);
      first = false;
    });
    p.buf += '}';
  } else if constexpr (fmt::is_formattable<T>::value) {
    fmt::format_to(std::back_inserter(p.buf), "{}", value);
  } else {
    p.buf += "?";
  }
}

// Whether a verb prints a value of the basic type T, rather than the bad-verb form.
template<typename T>
constexpr auto accepts_verb(int32_t verb) -> bool {
  if constexpr (std::is_same_v<T, bool>) {
    return verb == 't' || verb == 'v';
  } else if constexpr (go_integer<T>) {
    switch (verb) {
    case 'v':
    case 'd':
    case 'b':
    case 'o':
    case 'O':
    case 'x':
    case 'X':
    case 'c':
    case 'q':
    case 'U':
      return true;
    default:
      return false;
    }
  } else if constexpr (std::floating_point<T>) {
    switch (verb) {
    case 'v':
    case 'g':
    case 'e':
    case 'E':
    case 'f':
    case 'F':
      return true;
    default:
      return false;
    }
  } else if constexpr (std::is_same_v<T, std::string>) {
    return verb == 'v' || verb == 's' || verb == 'x' || verb == 'X' || verb == 'q';
  } else {
    return true;
  }
}

// Go's printArg: prints one operand with a verb. `depth` is 0 for an operand of the call.
template<typename T>
void print_arg(printer& p, const T& value, int32_t verb, int depth) {
  if constexpr (std::is_same_v<T, std::nullptr_t>) {
    // Go's nil as an operand, which the parameter of type any holds as a nil interface value.
    print_arg(p, any{}, verb, depth);
  } else if constexpr (interface_type<T>) {
    if (value == nullptr) {
      if (depth > 0 || verb == 'v' || verb == 'T') {
        p.buf += "<nil>";
      } else {
        bad_verb(p, verb, value);
      }
      return;
    }
    value._eo_type->print(p, value._eo_value.get(), verb, depth == 0 ? 0 : depth + 1);
  } else {
    if (depth == 0 && verb == 'T') {
      p.buf += type_of(value);
      return;
    }
    if constexpr (defined_type<T>) {
      // A defined type prints by its Error or String method, and otherwise as its underlying value,
      // except that a bad verb names the defined type, as Go's does.
      if (handle_methods(p, value, verb)) {
        return;
      }
      using U = typename T::_eo_underlying;
      if (!accepts_verb<U>(verb)) {
        bad_verb(p, verb, value);
        return;
      }
      print_arg(p, underlying(value), verb, depth);
    } else if constexpr (std::is_same_v<T, bool>) {
      if (verb == 't' || verb == 'v') {
        p.buf += value ? "true" : "false";
      } else {
        bad_verb(p, verb, value);
      }
    } else if constexpr (go_integer<T>) {
      print_integer(p, value, verb);
    } else if constexpr (std::floating_point<T>) {
      print_float(p, value, verb);
    } else if constexpr (std::is_same_v<T, std::string>) {
      if (!print_string(p, value, verb)) {
        bad_verb(p, verb, value);
      }
    } else if constexpr (is_string_like<T>) {
      print_arg(p, std::string{std::string_view{value}}, verb, depth);
    } else {
      if (handle_methods(p, value, verb)) {
        return;
      }
      print_value(p, value, verb, depth);
    }
  }
}

// One operand of a Printf-like call, with its type erased.
struct operand {
  const void* value;
  void (*print)(printer& p, const void* value, int32_t verb, int depth);
  // The operand as an error, for Errorf's %w, or nil if it is not one.
  auto (*as_error)(const void* value) -> error;
};

template<typename T>
void print_operand(printer& p, const void* value, int32_t verb, int depth) {
  print_arg(p, *static_cast<const T*>(value), verb, depth);
}

template<typename T>
auto operand_error(const void* value) -> error {
  const auto& v = *static_cast<const T*>(value);
  if constexpr (std::is_same_v<T, error>) {
    return v;
  } else if constexpr (interface_type<T>) {
    return type_assert_ok<error>(v).first;
  } else if constexpr (go_type<T>) {
    if constexpr (method_position<T, std::string()>("Error") != no_method) {
      return error{v};
    } else {
      return nullptr;
    }
  } else {
    return nullptr;
  }
}

template<typename T>
auto make_operand(const T& value) -> operand {
  return {&value, &print_operand<T>, &operand_error<T>};
}

// Go's doPrintf, without flags, width, precision, and argument indexes. The positions of the
// operands of %w are added to `wrapped`.
inline void print_format(
  printer& p, const std::string& format, std::span<const operand> operands, std::vector<std::size_t>* wrapped) {
  auto next = std::size_t{0};
  for (std::size_t i = 0; i < format.size();) {
    if (format[i] != '%') {
      p.buf += format[i++];
      continue;
    }
    ++i;
    if (i >= format.size()) {
      p.buf += "%!(NOVERB)";
      break;
    }
    auto [r, n] = decode_rune(format, i);
    i += n;
    auto verb = static_cast<int32_t>(r);
    if (verb == '%') {
      p.buf += '%';
      continue;
    }
    if (next >= operands.size()) {
      p.buf += "%!";
      append_rune(p.buf, verb);
      p.buf += "(MISSING)";
      continue;
    }
    if (verb == 'w' && wrapped != nullptr) {
      wrapped->push_back(next);
    }
    const auto& o = operands[next++];
    o.print(p, o.value, verb, 0);
  }
  if (next < operands.size()) {
    p.buf += "%!(EXTRA ";
    for (auto j = next; j < operands.size(); ++j) {
      if (j > next) {
        p.buf += ", ";
      }
      // The type and %v of the operand, which is <nil> for a nil interface value.
      auto typed = printer{};
      operands[j].print(typed, operands[j].value, 'T', 0);
      if (typed.buf != "<nil>") {
        p.buf += typed.buf;
        p.buf += '=';
      }
      operands[j].print(p, operands[j].value, 'v', 0);
    }
    p.buf += ')';
  }
}

template<typename... T>
auto sprintf(const std::string& format, const T&... args) -> std::string {
  auto p = printer{};
  const auto operands = std::array<operand, sizeof...(T)>{make_operand(args)...};
  print_format(p, format, operands, nullptr);
  return std::move(p.buf);
}

// Whether Go's Sprint would see the operand as a string, which gets no space next to another string.
template<typename T>
auto is_string_operand(const T& value) -> bool {
  if constexpr (interface_type<T>) {
    return value._eo_type == &descriptor<std::string>;
  } else {
    return is_string_like<T>;
  }
}

template<typename... T>
auto sprint(bool line, const T&... args) -> std::string {
  auto p = printer{};
  auto first = true;
  auto previous_string = false;
  auto print = [&](const auto& arg) {
    auto string = is_string_operand(arg);
    // Println separates every pair of operands, and Print only those where neither is a string.
    if (!first && (line || (!string && !previous_string))) {
      p.buf += ' ';
    }
    print_arg(p, arg, 'v', 0);
    first = false;
    previous_string = string;
  };
  (print(args), ...);
  if (line) {
    p.buf += '\n';
  }
  return std::move(p.buf);
}

// Go's fmt functions write each call to the unbuffered os.Stdout with a single write. Write the whole
// text, then flush it, so output is not held in a stdio buffer and lines from concurrent goroutines
// do not interleave.
inline void write_text(std::FILE* f, const std::string& text) {
  std::fwrite(text.data(), 1, text.size(), f);
  // runtime::exit skips stdout while another thread holds its lock, so this flush is what keeps a
  // goroutine's last line before os::Exit.
  std::fflush(f);
}

inline auto print_panic_value(const any& value) -> std::string {
  auto p = printer{};
  print_arg(p, value, 'v', 0);
  return std::move(p.buf);
}

// %v of a value, as Go's fmt prints it. `top` is true for an operand of Println.
template<typename T>
struct printed {
  const T& value;
  bool top;
};

template<typename T>
auto go_print(const T& value, bool top = false) -> printed<T> {
  return {value, top};
}

} // namespace eo::detail
