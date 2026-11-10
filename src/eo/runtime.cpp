// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/core.h>

#include <boost/exception/diagnostic_information.hpp>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <unistd.h>

namespace eo::runtime {

boost::asio::thread_pool execution_context = []() {
  auto maxprocs = std::max<std::size_t>(1, std::thread::hardware_concurrency());
  if (auto env = std::getenv("EOMAXPROCS")) {
    std::size_t configured{};
    std::string_view value{env};
    auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), configured);
    if (ec == std::errc{} && ptr == value.data() + value.size() && configured >= 1) {
      maxprocs = configured;
    }
  }
  return boost::asio::thread_pool{maxprocs};
}();

namespace {

  std::atomic<bool> panicking{false};

  // Flush a standard stream unless another thread holds its lock, for example a goroutine blocked
  // writing to a pipe that nobody reads. Waiting for that thread could take forever. Its write is
  // lost, as it would be in Go, and every earlier Println has already been flushed.
  void flush_unless_locked(std::FILE* f, std::ostream* os) {
    if (ftrylockfile(f) == 0) {
      if (os) {
        os->flush();
      }
      std::fflush(f);
      funlockfile(f);
    }
  }

  // Go writes the panic report straight to file descriptor 2, without taking any stream lock.
  void write_stderr(std::string_view s) noexcept {
    while (!s.empty()) {
      auto n = ::write(STDERR_FILENO, s.data(), s.size());
      if (n < 0) {
        if (errno == EINTR) {
          continue;
        }
        return;
      }
      s.remove_prefix(static_cast<std::size_t>(n));
    }
  }

  auto panic_message(std::exception_ptr eptr) -> std::string {
    try {
      std::rethrow_exception(eptr);
    } catch (const boost::exception& e) {
      return boost::diagnostic_information(e);
    } catch (const std::exception& e) {
      return e.what();
    } catch (...) {
      return "unknown exception";
    }
  }

} // namespace

void exit(int code) {
  // std::exit would run static destructors, and the thread_pool destructor joins worker threads
  // that may include the caller or be blocked in user code. Only the standard output streams are
  // flushed: fflush(nullptr) locks every open FILE, including stdin while another goroutine is
  // blocked reading it. std::cerr is not flushed through the stream, because flushing it first
  // flushes the tied std::cout; it is unit-buffered and has nothing pending.
  flush_unless_locked(stdout, &std::cout);
  flush_unless_locked(stderr, nullptr);
  std::_Exit(code);
}

void main_returned() {
  // Go's runtime.main does not exit while another goroutine is panicking, so that the panic ends
  // the program with status 2.
  // Like Go's runtime.main: give goroutines that are running deferred calls for a panic a moment to
  // finish, because they may still end the program with status 2.
  for (auto i = 0; i < 1000 && eo::detail::running_panic_defers.load() != 0; ++i) {
    std::this_thread::sleep_for(std::chrono::microseconds{100});
  }
  if (panicking.load()) {
    for (;;) {
      std::this_thread::sleep_for(std::chrono::hours{1});
    }
  }
  exit(0);
}

void fatal_panic(std::exception_ptr eptr) noexcept {
  panicking.store(true);
  // Never unlocked: a later panic on another thread waits here until the first one exits.
  static std::mutex reporting;
  reporting.lock();
  try {
    auto message = panic_message(eptr);
    if (!message.starts_with("panic: ")) {
      write_stderr("panic: ");
    }
    write_stderr(message);
  } catch (...) {
    write_stderr("panic: unknown exception");
  }
  write_stderr("\n");
  exit(2);
}

} // namespace eo::runtime
