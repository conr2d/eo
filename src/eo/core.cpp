#include <eo/core.h>

namespace eo {

// boost::asio::thread_pool executor{std::thread::hardware_concurrency()};
boost::asio::thread_pool executor{4};

} // namespace eo
