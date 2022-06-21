#include <eo/core.h>
#include <thread>

namespace eo::runtime {

boost::asio::thread_pool executor = []() {
  auto maxprocs = std::thread::hardware_concurrency();
  if (auto env = std::getenv("EOMAXPROCS")) {
    auto _maxprocs = std::stoull(env);
    if (_maxprocs >= 1) {
      maxprocs = _maxprocs;
    }
  }
  return boost::asio::thread_pool{maxprocs};
}();

} // namespace eo::runtime

namespace eo {

// XXX: default_chan should be declared below eo::runtime::executor on the same file,
// or it can cause segfault because of the order of initialization.
default_chan_type default_chan{};

} // namespace eo
