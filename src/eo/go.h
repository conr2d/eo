// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <boost/asio/detail/config.hpp>
#if defined(__clang__) && !defined(BOOST_ASIO_HAS_CO_AWAIT)
#  if __has_include(<coroutine>)
#    define BOOST_ASIO_HAS_CO_AWAIT 1
#  endif
#endif

#include <eo/runtime.h>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/as_tuple.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/use_awaitable.hpp>

namespace eo {

template<typename T = void>
using func = boost::asio::awaitable<T>;

using boost::asio::use_awaitable;

inline auto eoroutine = boost::asio::experimental::as_tuple(use_awaitable);

using namespace boost::asio::experimental::awaitable_operators;

template<typename F>
concept Awaitable = requires(F&& f) {
  boost::asio::co_spawn(runtime::execution_context, std::forward<F>(f), boost::asio::detached);
};

template<typename Executor, Awaitable F, typename CompletionToken>
void go(Executor& ex, F&& f, CompletionToken&& ct) {
  boost::asio::co_spawn(ex, std::forward<F>(f), std::forward<CompletionToken>(ct));
}

template<typename Executor, typename F>
void go(Executor& ex, F&& f) {
  boost::asio::post(ex, std::forward<F>(f));
}

template<typename Executor, Awaitable F>
void go(Executor& ex, F&& f) {
  go(ex, std::forward<F>(f), boost::asio::detached);
}

template<Awaitable F, typename CompletionToken>
void go(F&& f, CompletionToken&& ct) {
  go(runtime::execution_context, std::forward<F>(f), std::forward<CompletionToken>(ct));
}

template<typename F>
void go(F&& f) {
  go(runtime::execution_context, std::forward<F>(f));
}

} // namespace eo
