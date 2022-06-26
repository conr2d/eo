#pragma once
#include <eo/chan.h>
#include <boost/asio/steady_timer.hpp>

namespace eo::time {

struct Timer {
  boost::asio::steady_timer timer{runtime::executor};
  chan<> ch = make_chan(1);

  Timer(const std::chrono::steady_clock::duration& d) {
    reset(d);
  }

  ~Timer() {
    timer.cancel();
  }

  void reset(const std::chrono::steady_clock::duration& d) {
    timer.cancel();
    timer.expires_after(d);
    timer.async_wait([this, d{d}](boost::system::error_code ec) {
      if (ec)
        return;
      ch.try_send(boost::system::error_code{}, std::monostate{});
    });
  }
};

} // namespace eo::time
