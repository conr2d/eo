// This file is part of NOIR.
//
// Copyright (c) 2022 Haderech Pte. Ltd.
// SPDX-License-Identifier: AGPL-3.0-or-later
//
#pragma once
#include <noir/core/error.h>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/outcome/result.hpp>

namespace noir {

using namespace BOOST_OUTCOME_V2_NAMESPACE;

// NOTE:
// `std::expected` doesn't support construction from Error type,
// so it needs to wrap an error like `return std::unexpected(E);` to propagate it.
// `Result<T, E>` derived from `boost::outcome_v2::result<T, E>` allows construction from error.
//
// However, `boost::outcome_v2::result` doesn't have a default constructor by its design,
// so it cannot be used as a return type of `boost::asio::awaitable<R>` function.
// `boost::asio::co_spawn` calls completion token with `void(std::exception_ptr, R>` and
// `R` must be default constructible.
template<typename T, typename E = Error, typename NoValuePolicy = policy::default_policy<T, E, void>>
class Result : public basic_result<T, E, NoValuePolicy> {
public:
  using basic_result<T, E, NoValuePolicy>::basic_result;

  template<typename U>
  Result(std::in_place_type_t<U> _): basic_result<T, E, NoValuePolicy>(_) {}

private:
  Result(): basic_result<T, E, NoValuePolicy>(E()) {}

  // HACK: workaround for making boost::asio::co_spawn work with boost::asio::awaitable<Result<T, E>>
  template<typename U, typename Executor, typename F, typename Handler>
  friend boost::asio::awaitable<boost::asio::detail::awaitable_thread_entry_point, Executor>
  boost::asio::detail::co_spawn_entry_point(boost::asio::awaitable<U, Executor>*, Executor ex, F f, Handler handler);
};

} // namespace noir
