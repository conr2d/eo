// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/core.h>
#include <eo/io/fs.h>
#include <eo/syscall.h>

#include <string>
#include <string_view>
#include <tuple>

namespace eo::os {

// Go's os.SyscallError: an error from a system call, and the call's name.
struct SyscallError {
  static constexpr std::string_view _eo_name = "os.SyscallError";

  std::string Syscall{};
  error Err{};

  auto operator==(const SyscallError&) const -> bool = default;

  static auto Error(ptr<SyscallError> e) -> std::string {
    return e->Syscall + ": " + e->Err.Error();
  }

  static auto Unwrap(ptr<SyscallError> e) -> error {
    return e->Err;
  }

  // Timeout reports whether this error represents a timeout.
  static auto Timeout(ptr<SyscallError> e) -> bool {
    auto [t, ok] = eo::type_assert_ok<fs::detail::timeout>(e->Err);
    return ok && t.Timeout();
  }

  using _eo_methods = eo::methods<eo::method<&SyscallError::Error, "Error">,
    eo::method<&SyscallError::Timeout, "Timeout">,
    eo::method<&SyscallError::Unwrap, "Unwrap">>;
};

// Go's os.NewSyscallError: a *SyscallError with the call's name and err, or nil if err is nil.
inline auto NewSyscallError(std::string syscall, error err) -> error {
  if (err == nullptr) {
    return nullptr;
  }
  return New<SyscallError>(SyscallError{std::move(syscall), std::move(err)});
}

// Go's os.Getenv, LookupEnv, Setenv, and Unsetenv.
inline auto Getenv(const std::string& key) -> std::string {
  return std::get<0>(syscall::Getenv(key));
}

inline auto LookupEnv(const std::string& key) -> std::tuple<std::string, bool> {
  return syscall::Getenv(key);
}

inline auto Setenv(const std::string& key, const std::string& value) -> error {
  if (auto err = syscall::Setenv(key, value); err != nullptr) {
    return NewSyscallError("setenv", err);
  }
  return nullptr;
}

inline auto Unsetenv(const std::string& key) -> error {
  return syscall::Unsetenv(key);
}

auto ExpandEnv(const std::string& s) -> std::string;

// Go's os.Exit: ends the process at once with the given status, from any goroutine. Deferred calls
// do not run.
[[noreturn]] void Exit(int code);

} // namespace eo::os
