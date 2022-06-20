#include <eo/core.h>

namespace eo {

// boost::asio::thread_pool executor{std::thread::hardware_concurrency()};
boost::asio::thread_pool executor{4};

default_chan_type default_chan{};

} // namespace eo
