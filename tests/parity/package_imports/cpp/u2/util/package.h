#pragma once

#include <eo/core.h>

#include <string>
#include <string_view>

using namespace eo;

namespace github_com_conr2d_eo_tests_parity_package_imports_u2_util {

namespace _eo_package = ::github_com_conr2d_eo_tests_parity_package_imports_u2_util;

struct Box;

struct Box {
  static constexpr std::string_view _eo_name = "util.Box";

  std::string Label{};

  auto operator==(const Box&) const -> bool = default;
};

auto Name() -> std::string;

inline func<> eo_init() {
  co_return;
}

} // namespace github_com_conr2d_eo_tests_parity_package_imports_u2_util
