#pragma once
#include <eo/go.h>
#include <future>

namespace eo {

template<typename T>
auto invoke(func<T> f) {
  auto promise = std::promise<T>{};
  auto future = promise.get_future();
  if constexpr (std::is_void_v<T>) {
    go(std::forward<func<>>(f), [promise{std::move(promise)}](std::exception_ptr ep) mutable {
      if (ep) {
        // TODO: exception handling
      }
      promise.set_value();
    });
    future.get();
    return;
  } else {
    go(std::forward<func<T>>(f), [promise{std::move(promise)}](std::exception_ptr ep, T v) mutable {
      if (ep) {
        // TODO: exception handling
      }
      promise.set_value(std::forward<T>(v));
    });
    return future.get();
  }
}

template<typename F, typename... Args>
auto invoke(F&& f, Args&&... args) {
  return invoke(f(std::forward<Args>(args)...));
}

} // namespace eo
