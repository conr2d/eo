// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once

#include <eo/core.h>
#include <eo/errors.h>
#include <eo/io.h>
#include <eo/runtime.h>

#include <sys/random.h>

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <tuple>

// Go's crypto/rand: Reader and Read, ported from Go 1.26's rand.go and the parts of
// crypto/internal/rand and crypto/internal/sysrand that they call outside FIPS 140 mode. The bytes
// come from getrandom(2), as Go's do on Linux. Go falls back to /dev/urandom on kernels before 3.17,
// and Eo does not.

namespace eo::rand {

namespace detail {

  // Go's sysrand.Read: fill b from the operating system. A failure is fatal, with Go's message, and
  // a deferred recover() cannot stop it.
  inline void sysrandRead(slice<uint8_t> b) {
    auto* p = b.data();
    auto n = static_cast<size_t>(b.size());
    while (n > 0) {
      // getrandom returns at most 33554431 bytes at a time, as Go's loop allows for.
      auto got = ::getrandom(p, n, 0);
      if (got < 0) {
        if (errno == EINTR) {
          continue;
        }
        eo::runtime::fatal_error(
          std::string("crypto/rand: failed to read random data (see https://go.dev/issue/66821): ") +
          std::strerror(errno));
      }
      p += got;
      n -= static_cast<size_t>(got);
    }
  }

  // Go's crypto/internal/rand.reader, the dynamic type of the default Reader. Its Read never fails
  // and always fills b.
  struct reader {
    static constexpr std::string_view _eo_name = "rand.reader";

    auto operator==(const reader&) const -> bool = default;

    static auto Read(reader, slice<uint8_t> b) -> std::tuple<int64_t, error> {
      sysrandRead(b);
      return {eo::len(b), nullptr};
    }

    using _eo_methods = eo::methods<eo::method<&reader::Read, "Read">>;
  };

  // Go's crypto/internal/rand.IsDefaultReader.
  inline auto IsDefaultReader(const io::Reader& r) -> bool {
    return eo::holds<reader>(r);
  }

} // namespace detail

// Go's rand.Reader, a shared instance of a cryptographically secure random number generator. A
// program may replace it, and Read then reads from the replacement.
inline io::Reader Reader = io::Reader(detail::reader{});

// Go's rand.Read: fill b with cryptographically secure random bytes. It never returns an error and
// always fills b. If Reader was replaced and reading from it fails, the failure is fatal, as in Go.
inline auto Read(slice<uint8_t> b) -> std::tuple<int64_t, error> {
  error err{};
  if (detail::IsDefaultReader(Reader)) {
    detail::sysrandRead(b);
  } else {
    auto bb = eo::make_slice<uint8_t>(eo::len(b));
    std::tie(std::ignore, err) = io::ReadFull(Reader, bb);
    eo::copy(b, bb);
  }
  if (err != nullptr) {
    eo::runtime::fatal_error(
      "crypto/rand: failed to read random data (see https://go.dev/issue/66821): " + err.Error());
  }
  return {eo::len(b), nullptr};
}

namespace detail {

  // Go's crypto/internal/rand.CustomReader with the default GODEBUG of Go 1.26, where
  // cryptocustomrand is 0: the reader that crypto functions use instead of the one they are given,
  // which is always the default one.
  inline auto CustomReader(const io::Reader&) -> io::Reader {
    return io::Reader(reader{});
  }

} // namespace detail

} // namespace eo::rand
