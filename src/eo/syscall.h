// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once

#include <eo/core.h>
#include <eo/errors.h>
#include <eo/io/fs.h>

#include <cstdint>
#include <string>
#include <string_view>

// Go's syscall, in part: Errno, with its methods and the messages of Go 1.26's zerrors_linux_amd64.go,
// and the environment. The messages are Go's, which are not always the C library's. The Errno constants, such
// as ENOENT, are not declared: C's <cerrno> defines their names as macros.

namespace eo::syscall {

// Go's syscall.Errno, an error number of the system.
struct Errno : eo::defined<Errno, uint64_t> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "syscall.Errno";

  static auto Error(Errno e) -> std::string;
  static auto Is(Errno e, error target) -> bool;
  static auto Temporary(Errno e) -> bool;
  static auto Timeout(Errno e) -> bool;

  using _eo_methods = eo::methods<eo::method<&Errno::Error, "Error">,
    eo::method<&Errno::Is, "Is">,
    eo::method<&Errno::Temporary, "Temporary">,
    eo::method<&Errno::Timeout, "Timeout">>;
};

namespace detail {

  // The numbers of the Errno constants that the methods compare with. C's <cerrno> defines the
  // constants' names as macros, so they are not declared under Go's names.
  inline constexpr uint64_t eacces = 13;
  inline constexpr uint64_t einval = 22;
  inline constexpr uint64_t eperm = 1;
  inline constexpr uint64_t eexist = 17;
  inline constexpr uint64_t enotempty = 39;
  inline constexpr uint64_t enoent = 2;
  inline constexpr uint64_t enosys = 38;
  inline constexpr uint64_t enotsup = 95;
  inline constexpr uint64_t eopnotsupp = 95;
  inline constexpr uint64_t eagain = 11;
  inline constexpr uint64_t ewouldblock = 11;
  inline constexpr uint64_t etimedout = 110;
  inline constexpr uint64_t eintr = 4;
  inline constexpr uint64_t emfile = 24;
  inline constexpr uint64_t enfile = 23;

  struct errno_message {
    uint64_t number;
    std::string_view message;
  };

  inline constexpr errno_message errno_messages[] = {
    {1, "operation not permitted"},
    {2, "no such file or directory"},
    {3, "no such process"},
    {4, "interrupted system call"},
    {5, "input/output error"},
    {6, "no such device or address"},
    {7, "argument list too long"},
    {8, "exec format error"},
    {9, "bad file descriptor"},
    {10, "no child processes"},
    {11, "resource temporarily unavailable"},
    {12, "cannot allocate memory"},
    {13, "permission denied"},
    {14, "bad address"},
    {15, "block device required"},
    {16, "device or resource busy"},
    {17, "file exists"},
    {18, "invalid cross-device link"},
    {19, "no such device"},
    {20, "not a directory"},
    {21, "is a directory"},
    {22, "invalid argument"},
    {23, "too many open files in system"},
    {24, "too many open files"},
    {25, "inappropriate ioctl for device"},
    {26, "text file busy"},
    {27, "file too large"},
    {28, "no space left on device"},
    {29, "illegal seek"},
    {30, "read-only file system"},
    {31, "too many links"},
    {32, "broken pipe"},
    {33, "numerical argument out of domain"},
    {34, "numerical result out of range"},
    {35, "resource deadlock avoided"},
    {36, "file name too long"},
    {37, "no locks available"},
    {38, "function not implemented"},
    {39, "directory not empty"},
    {40, "too many levels of symbolic links"},
    {42, "no message of desired type"},
    {43, "identifier removed"},
    {44, "channel number out of range"},
    {45, "level 2 not synchronized"},
    {46, "level 3 halted"},
    {47, "level 3 reset"},
    {48, "link number out of range"},
    {49, "protocol driver not attached"},
    {50, "no CSI structure available"},
    {51, "level 2 halted"},
    {52, "invalid exchange"},
    {53, "invalid request descriptor"},
    {54, "exchange full"},
    {55, "no anode"},
    {56, "invalid request code"},
    {57, "invalid slot"},
    {59, "bad font file format"},
    {60, "device not a stream"},
    {61, "no data available"},
    {62, "timer expired"},
    {63, "out of streams resources"},
    {64, "machine is not on the network"},
    {65, "package not installed"},
    {66, "object is remote"},
    {67, "link has been severed"},
    {68, "advertise error"},
    {69, "srmount error"},
    {70, "communication error on send"},
    {71, "protocol error"},
    {72, "multihop attempted"},
    {73, "RFS specific error"},
    {74, "bad message"},
    {75, "value too large for defined data type"},
    {76, "name not unique on network"},
    {77, "file descriptor in bad state"},
    {78, "remote address changed"},
    {79, "can not access a needed shared library"},
    {80, "accessing a corrupted shared library"},
    {81, ".lib section in a.out corrupted"},
    {82, "attempting to link in too many shared libraries"},
    {83, "cannot exec a shared library directly"},
    {84, "invalid or incomplete multibyte or wide character"},
    {85, "interrupted system call should be restarted"},
    {86, "streams pipe error"},
    {87, "too many users"},
    {88, "socket operation on non-socket"},
    {89, "destination address required"},
    {90, "message too long"},
    {91, "protocol wrong type for socket"},
    {92, "protocol not available"},
    {93, "protocol not supported"},
    {94, "socket type not supported"},
    {95, "operation not supported"},
    {96, "protocol family not supported"},
    {97, "address family not supported by protocol"},
    {98, "address already in use"},
    {99, "cannot assign requested address"},
    {100, "network is down"},
    {101, "network is unreachable"},
    {102, "network dropped connection on reset"},
    {103, "software caused connection abort"},
    {104, "connection reset by peer"},
    {105, "no buffer space available"},
    {106, "transport endpoint is already connected"},
    {107, "transport endpoint is not connected"},
    {108, "cannot send after transport endpoint shutdown"},
    {109, "too many references: cannot splice"},
    {110, "connection timed out"},
    {111, "connection refused"},
    {112, "host is down"},
    {113, "no route to host"},
    {114, "operation already in progress"},
    {115, "operation now in progress"},
    {116, "stale file handle"},
    {117, "structure needs cleaning"},
    {118, "not a XENIX named type file"},
    {119, "no XENIX semaphores available"},
    {120, "is a named type file"},
    {121, "remote I/O error"},
    {122, "disk quota exceeded"},
    {123, "no medium found"},
    {124, "wrong medium type"},
    {125, "operation canceled"},
    {126, "required key not available"},
    {127, "key has expired"},
    {128, "key has been revoked"},
    {129, "key was rejected by service"},
    {130, "owner died"},
    {131, "state not recoverable"},
    {132, "operation not possible due to RF-kill"},
  };

} // namespace detail

inline auto Errno::Error(Errno e) -> std::string {
  auto n = uint64_t(e);
  for (const auto& m : detail::errno_messages) {
    if (m.number == n && !m.message.empty()) {
      return std::string(m.message);
    }
  }
  return "errno " + std::to_string(int64_t(n));
}

inline auto Errno::Is(Errno e, error target) -> bool {
  if (target == fs::ErrPermission) {
    return uint64_t(e) == detail::eacces || uint64_t(e) == detail::eperm;
  }
  if (target == fs::ErrExist) {
    return uint64_t(e) == detail::eexist || uint64_t(e) == detail::enotempty;
  }
  if (target == fs::ErrNotExist) {
    return uint64_t(e) == detail::enoent;
  }
  if (target == errors::ErrUnsupported) {
    return uint64_t(e) == detail::enosys || uint64_t(e) == detail::enotsup || uint64_t(e) == detail::eopnotsupp;
  }
  return false;
}

inline auto Errno::Timeout(Errno e) -> bool {
  return uint64_t(e) == detail::eagain || uint64_t(e) == detail::ewouldblock || uint64_t(e) == detail::etimedout;
}

inline auto Errno::Temporary(Errno e) -> bool {
  return uint64_t(e) == detail::eintr || uint64_t(e) == detail::emfile || uint64_t(e) == detail::enfile ||
    Errno::Timeout(e);
}

// Go's syscall.Getenv, Setenv, and Unsetenv, over the C library's environment, which Go's runtime
// also updates. Setenv returns EINVAL for an empty key, a key with '=' or NUL, and a value with NUL.
auto Getenv(const std::string& key) -> std::tuple<std::string, bool>;
auto Setenv(const std::string& key, const std::string& value) -> error;
auto Unsetenv(const std::string& key) -> error;

} // namespace eo::syscall
