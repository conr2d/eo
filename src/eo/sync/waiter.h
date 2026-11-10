// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/go.h>

#include <boost/asio/any_completion_handler.hpp>
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/associated_executor.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/post.hpp>

#include <utility>

namespace eo::sync::detail {

// Gives tests access to the waiter queues. Declared here, defined only by tests.
struct probe;

// A goroutine suspended in a sync operation: its completion handler and the executor it resumes on.
struct waiter {
  boost::asio::any_io_executor executor;
  boost::asio::any_completion_handler<void()> handler;
};

// Resumes a waiter on its own executor, never inline in the goroutine that released it.
inline void resume(waiter w) {
  boost::asio::post(w.executor, [handler = std::move(w.handler)]() mutable { std::move(handler)(); });
}

// Suspends the calling goroutine. enqueue(waiter&&) runs at once and either keeps the waiter, to
// resume it later, or returns it to resume the goroutine now.
template<typename Enqueue>
auto suspend(Enqueue enqueue) -> func<> {
  return boost::asio::async_initiate<decltype(boost::asio::use_awaitable), void()>(
    [enqueue = std::move(enqueue)](auto handler) mutable {
      auto w = waiter{
        boost::asio::get_associated_executor(handler, runtime::execution_context.get_executor()),
        std::move(handler),
      };
      if (auto ready = enqueue(std::move(w))) {
        resume(std::move(*ready));
      }
    },
    boost::asio::use_awaitable);
}

} // namespace eo::sync::detail
