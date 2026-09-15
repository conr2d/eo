// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/chan.h>
#include <eo/select.h>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/use_future.hpp>

#include <stdexcept>

namespace asio = boost::asio;

void check(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

auto select_with_leading_default(eo::chan<int> ready) -> eo::func<> {
  check(co_await (ready << 7), "buffered send should succeed");

  auto select = eo::Select{eo::CaseDefault{}, *ready};
  auto index = co_await select.index();

  check(index == 1, "ready communication should win over a leading default case");
  check(co_await select.process<1>() == 7, "selected receive should preserve its value");
}

auto select_with_middle_default(eo::chan<int> idle, eo::chan<int> ready) -> eo::func<> {
  check(co_await (ready << 11), "buffered send should succeed");

  auto select = eo::Select{*idle, eo::CaseDefault{}, *ready};
  auto index = co_await select.index();

  check(index == 2, "ready communication should win over a middle default case");
  check(co_await select.process<2>() == 11, "selected receive should preserve its value");
}

auto select_leading_default_when_idle(eo::chan<int> idle) -> eo::func<> {
  auto select = eo::Select{eo::CaseDefault{}, *idle};
  auto index = co_await select.index();

  check(index == -1, "leading default should be selected when communication is not ready");
}

int main() {
  {
    asio::io_context io;
    eo::chan<int> ready{io.get_executor(), 1};
    auto result = asio::co_spawn(io, select_with_leading_default(ready), asio::use_future);
    io.run();
    result.get();
  }

  {
    asio::io_context io;
    eo::chan<int> idle{io.get_executor(), 1};
    eo::chan<int> ready{io.get_executor(), 1};
    auto result = asio::co_spawn(io, select_with_middle_default(idle, ready), asio::use_future);
    io.run();
    result.get();
  }

  {
    asio::io_context io;
    eo::chan<int> idle{io.get_executor(), 1};
    auto result = asio::co_spawn(io, select_leading_default_when_idle(idle), asio::use_future);
    io.run();
    result.get();
  }
}
