// This file is part of NOIR.
//
// Copyright (c) 2022 Haderech Pte. Ltd.
// SPDX-License-Identifier: AGPL-3.0-or-later
//
#pragma once
#include <fmt/core.h>
#include <map>
#include <optional>
#include <system_error>

namespace noir {

class Error;

class UserErrorRegistry {
public:
  std::string message(int condition);

  // message is not copied, so it MUST point out statically allocated memory address.
  Error register_error(std::string_view message);

private:
  std::map<int, std::string_view> messages;
  int counter = 0;
};

inline UserErrorRegistry& user_error_registry() {
  static UserErrorRegistry registry{};
  return registry;
}

class UserErrorCategory : public std::error_category {
public:
  const char* name() const noexcept override {
    return "user";
  }
  std::string message(int condition) const override {
    return user_error_registry().message(condition);
  }
};

inline const std::error_category& user_category() {
  static UserErrorCategory category{};
  return category;
}

class Error {
public:
  constexpr Error() noexcept = default;
  virtual ~Error() = default;

  // NOTE: Do not set default value for 2nd parameter (error_category),
  // it makes `noir::Error` constructible from int, and break implicit construction of Result<int> from value.
  constexpr Error(int ec, const std::error_category& ecat): value_(ec), category_(&ecat) {}

  template<typename T>
  requires(!std::is_same_v<T, Error> &&
    std::is_convertible_v<T, std::error_code>) // FIXME: clang-format 15 RequiresClausePosition: OwnLine
    constexpr Error(T&& err) {
    auto& ec = static_cast<const std::error_code&>(err);
    value_ = ec.value();
    category_ = &ec.category();
  }

  Error(std::string_view message): value_(-1), message_(message) {}

  template<typename... T>
  static constexpr auto format(fmt::format_string<T...> fmt, T&&... args) {
    return Error(fmt::vformat(fmt, fmt::make_format_args(args...)));
  }

  void assign(int ec, const std::error_category& ecat) noexcept {
    value_ = ec;
    category_ = &ecat;
  }

  void clear() noexcept {
    value_ = 0;
    category_ = &user_category();
  }

  int value() const noexcept {
    return value_;
  }

  const std::error_category& category() const noexcept {
    return *category_;
  }

  std::string message() const {
    if (message_) {
      return *message_;
    } else {
      return category().message(value());
    }
  }

  explicit operator bool() const noexcept {
    return value() != 0;
  }

  explicit operator std::error_code() noexcept {
    return std::error_code(value_, *category_);
  }
  explicit operator std::error_code() const noexcept {
    return std::error_code(value_, *category_);
  }

  bool operator==(const Error& err) const& noexcept {
    return category() == err.category() && value() == err.value() && message_ == err.message_;
  }

  bool operator!=(const Error& err) const& noexcept {
    return !(*this == err);
  }

  bool operator<(const Error& err) const& noexcept {
    return category() < err.category() || (category() == err.category() && value() < err.value()) ||
      (category() == err.category() && value() == err.value() && message_ < err.message_);
  }

private:
  int value_ = 0;
  const std::error_category* category_ = &user_category();
  std::optional<std::string> message_{};
};

inline std::string UserErrorRegistry::message(int condition) {
  if (messages.contains(condition)) {
    return std::string{messages[condition]};
  }
  return "runtime error";
}

inline Error UserErrorRegistry::register_error(std::string_view message) {
  messages[++counter] = message;
  return Error(counter, user_category());
}

inline std::exception_ptr make_exception_ptr(Error err) {
  return make_exception_ptr(std::error_code(err));
}

} // namespace noir

namespace std {

template<>
struct hash<noir::Error> {
  size_t operator()(const noir::Error& err) const noexcept {
    return static_cast<size_t>(err.value());
  }
};

} // namespace std

namespace fmt {

template<>
struct formatter<noir::Error> {
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }

  template<typename FormatContext>
  auto format(const noir::Error& err, FormatContext& ctx) -> decltype(ctx.out()) {
    return format_to(ctx.out(), "{}", err.message());
  }
};

} // namespace fmt
