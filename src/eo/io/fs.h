// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once

#include <eo/core.h>
#include <eo/errors.h>
#include <eo/unicode/utf8.h>

#include <string>
#include <string_view>

// Go's io/fs: the errors of the package, PathError, and ValidPath, from Go 1.26's fs.go. The FS,
// File, FileInfo, and DirEntry interfaces, FileMode, and the functions over a file system, such as
// ReadFile, ReadDir, Glob, and WalkDir, are not mapped.

namespace eo::fs {

// Go's fs.ErrInvalid, ErrPermission, ErrExist, ErrNotExist, and ErrClosed, which are those of
// internal/oserror, so that os returns the same values.
inline error ErrInvalid = errors::New("invalid argument");
inline error ErrPermission = errors::New("permission denied");
inline error ErrExist = errors::New("file already exists");
inline error ErrNotExist = errors::New("file does not exist");
inline error ErrClosed = errors::New("file already closed");

namespace detail {

  // The interface { Timeout() bool } that PathError.Timeout asserts its error to.
  struct timeout : eo::interface<timeout> {
    using interface::interface;
    static constexpr std::string_view _eo_name = "interface { Timeout() bool }";
    auto Timeout() const -> bool;
    using _eo_methods = eo::methods<eo::method<&timeout::Timeout, "Timeout">>;
  };

  inline auto timeout::Timeout() const -> bool {
    return _eo_call<&timeout::Timeout>();
  }

} // namespace detail

// Go's fs.PathError: an error and the operation and file path that caused it.
struct PathError {
  static constexpr std::string_view _eo_name = "fs.PathError";

  std::string Op{};
  std::string Path{};
  error Err{};

  auto operator==(const PathError&) const -> bool = default;

  static auto Error(ptr<PathError> e) -> std::string {
    return e->Op + " " + e->Path + ": " + e->Err.Error();
  }

  static auto Unwrap(ptr<PathError> e) -> error {
    return e->Err;
  }

  // Timeout reports whether this error represents a timeout.
  static auto Timeout(ptr<PathError> e) -> bool {
    auto [t, ok] = eo::type_assert_ok<detail::timeout>(e->Err);
    return ok && t.Timeout();
  }

  using _eo_methods = eo::methods<eo::method<&PathError::Error, "Error">,
    eo::method<&PathError::Unwrap, "Unwrap">,
    eo::method<&PathError::Timeout, "Timeout">>;
};

// Go's fs.ValidPath: whether name is a valid path name for Open, an unrooted, slash-separated
// sequence of path elements, none of which is empty, ".", or "..", or the name ".".
inline auto ValidPath(std::string name) -> bool {
  if (!utf8::ValidString(name)) {
    return false;
  }

  if (name == ".") {
    // special case
    return true;
  }

  // Iterate over elements in name, checking each.
  auto rest = std::string_view{name};
  for (;;) {
    auto i = size_t{0};
    while (i < rest.size() && rest[i] != '/') {
      i++;
    }
    auto elem = rest.substr(0, i);
    if (elem.empty() || elem == "." || elem == "..") {
      return false;
    }
    if (i == rest.size()) {
      return true; // reached clean ending
    }
    rest = rest.substr(i + 1);
  }
}

} // namespace eo::fs
