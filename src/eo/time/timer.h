// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/chan.h>
#include <boost/asio/steady_timer.hpp>

namespace eo::time {

struct Timer : public std::enable_shared_from_this<Timer> {
private:
  template<typename Executor>
  Timer(Executor& ex): timer(ex) {}

public:
  using time_point = std::chrono::system_clock::time_point;

  boost::asio::steady_timer timer;
  chan<time_point> c = make_chan<time_point>(1);
  bool expired;

  template<typename Executor>
  static auto create_with_executor(Executor& ex, const std::chrono::steady_clock::duration& d) -> std::shared_ptr<Timer> {
    auto timer = std::shared_ptr<Timer>(new Timer(ex));
    timer->reset(d);
    return timer;
  }

  static auto create(const std::chrono::steady_clock::duration& d) -> std::shared_ptr<Timer> {
    return create_with_executor(runtime::execution_context, d);
  }

  ~Timer() {
    timer.cancel();
  }

  void reset(const std::chrono::steady_clock::duration& d) {
    timer.cancel();
    timer.expires_after(d);
    expired = false;
    timer.async_wait([=, self{shared_from_this()}](boost::system::error_code ec) {
      self->expired = true;
      if (ec)
        return;
      self->c.try_send(boost::system::error_code{}, std::chrono::system_clock::now());
    });
  }

  auto stop() -> bool {
    if (expired) {
      return false;
    }
    timer.cancel();
    return !std::exchange(expired, true);
  }
};

const auto new_timer = Timer::create;

} // namespace eo::time
