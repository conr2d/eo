// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/core.h>
#include <eo/errors.h>
#include <eo/io.h>
#include <eo/print.h>

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdio>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace fmt {

// Go's fmt.State: what a Formatter's Format method prints to, with the flags, width, and precision of
// the verb. It is an io.Writer, so Format can print with Fprintf.
struct State : eo::interface<State> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "fmt.State";

  auto Write(eo::slice<uint8_t> b) const -> std::tuple<int64_t, eo::error>;
  auto Width() const -> std::tuple<int64_t, bool>;
  auto Precision() const -> std::tuple<int64_t, bool>;
  auto Flag(int64_t c) const -> bool;

  using _eo_methods = eo::methods<eo::method<&State::Write, "Write">,
    eo::method<&State::Width, "Width">,
    eo::method<&State::Precision, "Precision">,
    eo::method<&State::Flag, "Flag">>;
};

inline auto State::Write(eo::slice<uint8_t> b) const -> std::tuple<int64_t, eo::error> {
  return _eo_call<&State::Write>(std::move(b));
}

inline auto State::Width() const -> std::tuple<int64_t, bool> {
  return _eo_call<&State::Width>();
}

inline auto State::Precision() const -> std::tuple<int64_t, bool> {
  return _eo_call<&State::Precision>();
}

inline auto State::Flag(int64_t c) const -> bool {
  return _eo_call<&State::Flag>(c);
}

// Go's fmt.Formatter: a value that prints itself for every verb.
struct Formatter : eo::interface<Formatter> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "fmt.Formatter";

  void Format(State f, int32_t verb) const;

  using _eo_methods = eo::methods<eo::method<&Formatter::Format, "Format">>;
};

inline void Formatter::Format(State f, int32_t verb) const {
  _eo_call<&Formatter::Format>(std::move(f), verb);
}

// Go's *fmt.pp as a State: the printer of the call that prints. Write appends to its text without
// padding, and Flag reports the + and # of %v as Go's does.
struct pp {
  static constexpr std::string_view _eo_name = "fmt.pp";

  eo::detail::printer* p = nullptr;

  // The State that refers to s, which lives while Format runs.
  static auto _eo_state(pp& s) -> State {
    return State{eo::ref(s)};
  }

  static auto Write(eo::ptr<pp> s, eo::slice<uint8_t> b) -> std::tuple<int64_t, eo::error> {
    s->p->buf.append(b.begin(), b.end());
    return {b.size(), nullptr};
  }

  static auto WriteString(eo::ptr<pp> s, std::string str) -> std::tuple<int64_t, eo::error> {
    s->p->buf += str;
    return {static_cast<int64_t>(str.size()), nullptr};
  }

  static auto Width(eo::ptr<pp> s) -> std::tuple<int64_t, bool> {
    return {s->p->wid, s->p->flags.wid_present};
  }

  static auto Precision(eo::ptr<pp> s) -> std::tuple<int64_t, bool> {
    return {s->p->prec, s->p->flags.prec_present};
  }

  static auto Flag(eo::ptr<pp> s, int64_t c) -> bool {
    const auto& flags = s->p->flags;
    switch (c) {
    case '-':
      return flags.minus;
    case '+':
      return flags.plus || flags.plus_v;
    case '#':
      return flags.sharp || flags.sharp_v;
    case ' ':
      return flags.space;
    case '0':
      return flags.zero;
    default:
      return false;
    }
  }

  using _eo_methods = eo::methods<eo::method<&pp::Write, "Write">,
    eo::method<&pp::WriteString, "WriteString">,
    eo::method<&pp::Width, "Width">,
    eo::method<&pp::Precision, "Precision">,
    eo::method<&pp::Flag, "Flag">>;
};

// Go's fmt.Stringer.
struct Stringer : eo::interface<Stringer> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "fmt.Stringer";
  auto String() const -> std::string;
  using _eo_methods = eo::methods<eo::method<&Stringer::String, "String">>;
};

inline auto Stringer::String() const -> std::string {
  return _eo_call<&Stringer::String>();
}

namespace detail {

  // The operands are taken by value, as Go passes them. A reference would refer into a slice's backing
  // array, which another operand's call can free before the text is formatted.

  // Writes the text of one call to a std::FILE*, as Go's unbuffered os.Stdout and os.Stderr do.
  inline auto write_file(std::FILE* f, const std::string& text) -> std::tuple<int64_t, eo::error> {
    eo::detail::write_text(f, text);
    return {static_cast<int64_t>(text.size()), nullptr};
  }

  // Writes the text of one call with one Write, as Go's Fprint, Fprintln, and Fprintf do.
  inline auto write_writer(const eo::io::Writer& w, const std::string& text) -> std::tuple<int64_t, eo::error> {
    return w.Write(eo::bytes_from_string(text));
  }

  struct print_fn {
    template<typename... T>
    auto operator()(T... args) const -> std::tuple<int64_t, eo::error> {
      return write_file(stdout, eo::detail::sprint(false, args...));
    }
  };

  struct println_fn {
    template<typename... T>
    auto operator()(T... args) const -> std::tuple<int64_t, eo::error> {
      return write_file(stdout, eo::detail::sprint(true, args...));
    }
  };

  struct printf_fn {
    template<typename... T>
    auto operator()(const std::string& format, T... args) const -> std::tuple<int64_t, eo::error> {
      return write_file(stdout, eo::detail::sprintf(format, args...));
    }
  };

  struct fprint_fn {
    template<typename... T>
    auto operator()(std::FILE* f, T... args) const -> std::tuple<int64_t, eo::error> {
      return write_file(f, eo::detail::sprint(false, args...));
    }
    template<typename... T>
    auto operator()(eo::io::Writer w, T... args) const -> std::tuple<int64_t, eo::error> {
      return write_writer(w, eo::detail::sprint(false, args...));
    }
    // A nil writer, which Go panics on after formatting, when it calls Write.
    template<typename... T>
    auto operator()(std::nullptr_t, T... args) const -> std::tuple<int64_t, eo::error> {
      return (*this)(eo::io::Writer{}, std::move(args)...);
    }
  };

  struct fprintln_fn {
    template<typename... T>
    auto operator()(std::FILE* f, T... args) const -> std::tuple<int64_t, eo::error> {
      return write_file(f, eo::detail::sprint(true, args...));
    }
    template<typename... T>
    auto operator()(eo::io::Writer w, T... args) const -> std::tuple<int64_t, eo::error> {
      return write_writer(w, eo::detail::sprint(true, args...));
    }
    // A nil writer, which Go panics on after formatting, when it calls Write.
    template<typename... T>
    auto operator()(std::nullptr_t, T... args) const -> std::tuple<int64_t, eo::error> {
      return (*this)(eo::io::Writer{}, std::move(args)...);
    }
  };

  struct fprintf_fn {
    template<typename... T>
    auto operator()(std::FILE* f, const std::string& format, T... args) const -> std::tuple<int64_t, eo::error> {
      return write_file(f, eo::detail::sprintf(format, args...));
    }
    template<typename... T>
    auto operator()(eo::io::Writer w, const std::string& format, T... args) const -> std::tuple<int64_t, eo::error> {
      return write_writer(w, eo::detail::sprintf(format, args...));
    }
    template<typename... T>
    auto operator()(std::nullptr_t, const std::string& format, T... args) const -> std::tuple<int64_t, eo::error> {
      return (*this)(eo::io::Writer{}, format, std::move(args)...);
    }
  };

  struct sprint_fn {
    template<typename... T>
    auto operator()(T... args) const -> std::string {
      return eo::detail::sprint(false, args...);
    }
  };

  struct sprintln_fn {
    template<typename... T>
    auto operator()(T... args) const -> std::string {
      return eo::detail::sprint(true, args...);
    }
  };

  struct sprintf_fn {
    template<typename... T>
    auto operator()(const std::string& format, T... args) const -> std::string {
      return eo::detail::sprintf(format, args...);
    }
  };

  struct append_fn {
    template<typename... T>
    auto operator()(eo::slice<uint8_t> b, T... args) const -> eo::slice<uint8_t> {
      return eo::append_slice(std::move(b), eo::detail::sprint(false, args...));
    }
  };

  struct appendln_fn {
    template<typename... T>
    auto operator()(eo::slice<uint8_t> b, T... args) const -> eo::slice<uint8_t> {
      return eo::append_slice(std::move(b), eo::detail::sprint(true, args...));
    }
  };

  struct appendf_fn {
    template<typename... T>
    auto operator()(eo::slice<uint8_t> b, const std::string& format, T... args) const -> eo::slice<uint8_t> {
      return eo::append_slice(std::move(b), eo::detail::sprintf(format, args...));
    }
  };

} // namespace detail

// Function objects rather than function templates, so that a call with the results of another call,
// such as Go's fmt.Println(f()), translates to std::apply(fmt::Println, f()).
inline constexpr detail::print_fn Print{};
inline constexpr detail::println_fn Println{};
inline constexpr detail::printf_fn Printf{};
inline constexpr detail::fprint_fn Fprint{};
inline constexpr detail::fprintln_fn Fprintln{};
inline constexpr detail::fprintf_fn Fprintf{};
inline constexpr detail::sprint_fn Sprint{};
inline constexpr detail::sprintln_fn Sprintln{};
inline constexpr detail::sprintf_fn Sprintf{};
inline constexpr detail::append_fn Append{};
inline constexpr detail::appendln_fn Appendln{};
inline constexpr detail::appendf_fn Appendf{};

// Go's fmt.FormatString: the directive that the State and the verb were parsed from, with its flags,
// width, and precision but without operand indexes or *.
inline auto FormatString(State state, int32_t verb) -> std::string {
  auto b = std::string{"%"};
  for (auto c : std::string_view{" +-#0"}) {
    if (state.Flag(c)) {
      b += c;
    }
  }
  if (auto [w, ok] = state.Width(); ok) {
    b += std::to_string(w);
  }
  if (auto [p, ok] = state.Precision(); ok) {
    b += '.';
    b += std::to_string(p);
  }
  eo::detail::append_rune(b, verb);
  return b;
}

// fmt's formatters for Go values print them as Go's %v does inside another value.

template<typename T>
struct formatter<eo::detail::printed<T>> {
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.end();
  }
  template<typename FormatContext>
  auto format(const eo::detail::printed<T>& p, FormatContext& ctx) const {
    auto printer = eo::detail::printer{};
    eo::detail::print_arg(printer, p.value, 'v', p.top ? 0 : 1);
    return fmt::format_to(ctx.out(), "{}", printer.buf);
  }
};

template<typename K, typename V>
struct formatter<eo::map<K, V>> : formatter<eo::detail::printed<eo::map<K, V>>> {
  template<typename FormatContext>
  auto format(const eo::map<K, V>& value, FormatContext& ctx) const {
    return formatter<eo::detail::printed<eo::map<K, V>>>::format(eo::detail::go_print(value), ctx);
  }
};

template<typename T>
struct formatter<eo::slice<T>> : formatter<eo::detail::printed<eo::slice<T>>> {
  template<typename FormatContext>
  auto format(const eo::slice<T>& value, FormatContext& ctx) const {
    return formatter<eo::detail::printed<eo::slice<T>>>::format(eo::detail::go_print(value), ctx);
  }
};

template<eo::detail::go_struct T>
struct formatter<T> : formatter<eo::detail::printed<T>> {
  template<typename FormatContext>
  auto format(const T& value, FormatContext& ctx) const {
    return formatter<eo::detail::printed<T>>::format(eo::detail::go_print(value), ctx);
  }
};

template<typename T>
struct formatter<eo::ptr<T>> : formatter<eo::detail::printed<eo::ptr<T>>> {
  template<typename FormatContext>
  auto format(const eo::ptr<T>& value, FormatContext& ctx) const {
    return formatter<eo::detail::printed<eo::ptr<T>>>::format(eo::detail::go_print(value), ctx);
  }
};

template<eo::detail::interface_type T>
struct formatter<T> : formatter<eo::detail::printed<T>> {
  template<typename FormatContext>
  auto format(const T& value, FormatContext& ctx) const {
    return formatter<eo::detail::printed<T>>::format(eo::detail::go_print(value), ctx);
  }
};

// *fmt.wrapError, which Errorf returns for one %w.
struct wrapError {
  static constexpr std::string_view _eo_name = "fmt.wrapError";

  std::string msg;
  eo::error err;

  auto operator==(const wrapError&) const -> bool = default;

  static auto Error(eo::ptr<wrapError> e) -> std::string {
    return e->msg;
  }

  static auto Unwrap(eo::ptr<wrapError> e) -> eo::error {
    return e->err;
  }

  using _eo_methods = eo::methods<eo::method<&wrapError::Error, "Error">, eo::method<&wrapError::Unwrap, "Unwrap">>;
};

// *fmt.wrapErrors, which Errorf returns for several %w.
struct wrapErrors {
  static constexpr std::string_view _eo_name = "fmt.wrapErrors";

  std::string msg;
  eo::slice<eo::error> errs;

  static auto Error(eo::ptr<wrapErrors> e) -> std::string {
    return e->msg;
  }

  static auto Unwrap(eo::ptr<wrapErrors> e) -> eo::slice<eo::error> {
    return e->errs;
  }

  using _eo_methods = eo::methods<eo::method<&wrapErrors::Error, "Error">, eo::method<&wrapErrors::Unwrap, "Unwrap">>;
};

namespace detail {

  struct errorf_fn {
    template<typename... T>
    auto operator()(const std::string& format, T... args) const -> eo::error {
      auto p = eo::detail::printer{.wrap_errs = true};
      const auto operands = std::array<eo::detail::operand, sizeof...(T)>{eo::detail::make_operand(args)...};
      auto wrapped = std::vector<std::size_t>{};
      auto reordered = eo::detail::print_format(p, format, operands, &wrapped);
      if (wrapped.empty()) {
        return eo::errors::New(std::move(p.buf));
      }
      auto error_of = [&](std::size_t i) { return operands[i].as_error(operands[i].value); };
      if (wrapped.size() == 1) {
        return eo::New<wrapError>(wrapError{std::move(p.buf), error_of(wrapped[0])});
      }
      // Operand indexes can name an operand out of order or more than once. Go then sorts the
      // positions and wraps each operand once.
      if (reordered) {
        std::sort(wrapped.begin(), wrapped.end());
        wrapped.erase(std::unique(wrapped.begin(), wrapped.end()), wrapped.end());
      }
      auto errs = std::vector<eo::error>{};
      for (auto i : wrapped) {
        if (auto e = error_of(i); e != nullptr) {
          errs.push_back(std::move(e));
        }
      }
      auto s = eo::slice<eo::error>{};
      if (!errs.empty()) {
        s = eo::slice<eo::error>::allocate(static_cast<int64_t>(errs.size()), static_cast<int64_t>(errs.size()));
        std::move(errs.begin(), errs.end(), s.begin());
      }
      return eo::New<wrapErrors>(wrapErrors{std::move(p.buf), std::move(s)});
    }
  };

} // namespace detail

// fmt.Errorf: an error whose message is formatted as Sprintf formats it. Each %w operand that is an
// error is wrapped, so errors.Is, errors.As, and errors.Unwrap see it.
inline constexpr detail::errorf_fn Errorf{};

} // namespace fmt

namespace eo {

namespace fmt = ::fmt;

} // namespace eo
