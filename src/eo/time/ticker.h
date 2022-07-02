// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/chan.h>
#include <boost/asio/steady_timer.hpp>

namespace eo::time {

struct Ticker : public std::enable_shared_from_this<Ticker> {
  using time_point = std::chrono::system_clock::time_point;

  boost::asio::steady_timer timer{runtime::executor};
  chan<time_point> c = make_chan<time_point>(1);

  static auto create(const std::chrono::steady_clock::duration& d) -> std::shared_ptr<Ticker> {
    auto timer = std::make_shared<Ticker>();
    timer->reset(d);
    return timer;
  }

  template<typename Executor>
  static auto create_with_executor(const std::chrono::steady_clock::duration& d, Executor& executor) -> std::shared_ptr<Ticker> {
    auto timer = std::make_shared<Ticker>(executor);
    timer->reset(d);
    return timer;
  }

  Ticker() = default;

  template<typename Executor>
  Ticker(Executor& executor): timer(executor) {}

  ~Ticker() {
    timer.cancel();
  }

  void reset(const std::chrono::steady_clock::duration& d) {
    timer.cancel();
    timer.expires_after(d);
    timer.async_wait([=, self{shared_from_this()}](boost::system::error_code ec) {
      if (ec)
        return;
      self->c.try_send(boost::system::error_code{}, std::chrono::system_clock::now());
      self->reset(d);
    });
  }

  void stop() {
    timer.cancel();
  }
};

const auto new_ticker = Ticker::create;

} // namespace eo::time
