#pragma once
#include <eo/runtime.h>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/as_result.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/use_awaitable.hpp>

namespace eo {

template<typename Ret = void>
using func = boost::asio::awaitable<Ret>;

inline auto eoroutine = boost::asio::experimental::as_result(boost::asio::use_awaitable);

using namespace boost::asio::experimental::awaitable_operators;

template<typename F>
void go(F&& f) {
  boost::asio::post(runtime::executor, std::forward<F>(f));
}

template<typename F>
concept Awaitable = requires(F&& f) {
  boost::asio::co_spawn(runtime::executor, std::forward<F>(f), boost::asio::detached);
};

template<Awaitable F>
void go(F&& f) {
  boost::asio::co_spawn(runtime::executor, std::forward<F>(f), boost::asio::detached);
}

} // namespace eo
