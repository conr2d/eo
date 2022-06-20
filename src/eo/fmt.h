#pragma once
#include <eo/core.h>

#include <fmt/core.h>

namespace fmt {

template<typename T, typename... Ts>
void println(T&& format_str, Ts&&... args) {
  fmt::print(std::forward<T>(format_str), std::forward<Ts>(args)...);
  fmt::print("\n");
}

template<typename T, typename... Ts>
void fprintln(std::FILE* f, T&& format_str, Ts&&... args) {
  fmt::print(f, std::forward<T>(format_str), std::forward<Ts>(args)...);
  fmt::print(f, "\n");
}

} // namespace fmt

namespace eo {

namespace fmt = ::fmt;

} // namespace eo
