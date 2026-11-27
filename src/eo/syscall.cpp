// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/syscall.h>

#include <cstdlib>
#include <mutex>

namespace eo::syscall {

namespace {

  // Go's envLock. The C library's environment is not safe for concurrent use, so every access
  // takes it.
  std::mutex envLock;

} // namespace

auto Getenv(const std::string& key) -> std::tuple<std::string, bool> {
  std::lock_guard lock(envLock);
  if (auto* value = std::getenv(key.c_str())) {
    return {std::string(value), true};
  }
  return {std::string{}, false};
}

auto Setenv(const std::string& key, const std::string& value) -> error {
  if (key.empty()) {
    return Errno{detail::einval};
  }
  for (auto c : key) {
    if (c == '=' || c == 0) {
      return Errno{detail::einval};
    }
  }
  for (auto c : value) {
    if (c == 0) {
      return Errno{detail::einval};
    }
  }
  std::lock_guard lock(envLock);
  ::setenv(key.c_str(), value.c_str(), 1);
  return nullptr;
}

auto Unsetenv(const std::string& key) -> error {
  std::lock_guard lock(envLock);
  ::unsetenv(key.c_str());
  return nullptr;
}

} // namespace eo::syscall
