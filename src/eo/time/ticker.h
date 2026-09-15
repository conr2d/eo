// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/chan.h>
#include <boost/asio/steady_timer.hpp>

#include <mutex>

namespace eo::time {

struct Ticker : public std::enable_shared_from_this<Ticker> {
private:
  template<typename Executor>
  Ticker(Executor& ex): timer(ex) {}

  void schedule_locked(const std::chrono::steady_clock::duration& d, size_t current_generation) {
    timer.expires_after(d);
    timer.async_wait([self{shared_from_this()}, d, current_generation](boost::system::error_code ec) {
      std::lock_guard lock(self->state_mutex);
      if (ec || self->stopped || current_generation != self->generation)
        return;

      self->C.raw().try_send(boost::system::error_code{}, std::chrono::system_clock::now());
      self->schedule_locked(d, current_generation);
    });
  }

public:
  using time_point = std::chrono::system_clock::time_point;

  boost::asio::steady_timer timer;
  chan<time_point> C = make_chan<time_point>(1);

  template<typename Executor>
  static auto create_with_executor(Executor& ex,
    const std::chrono::steady_clock::duration& d) -> std::shared_ptr<Ticker> {
    auto timer = std::shared_ptr<Ticker>(new Ticker(ex));
    timer->Reset(d);
    return timer;
  }

  static auto create(const std::chrono::steady_clock::duration& d) -> std::shared_ptr<Ticker> {
    return create_with_executor(runtime::execution_context, d);
  }

  ~Ticker() {
    timer.cancel();
  }

  void Reset(const std::chrono::steady_clock::duration& d) {
    std::lock_guard lock(state_mutex);
    timer.cancel();
    stopped = false;
    const auto current_generation = ++generation;
    schedule_locked(d, current_generation);
  }

  void Stop() {
    std::lock_guard lock(state_mutex);
    stopped = true;
    ++generation;
    timer.cancel();
  }

private:
  std::mutex state_mutex;
  bool stopped = false;
  size_t generation = 0;
};

const auto NewTicker = Ticker::create;

} // namespace eo::time
