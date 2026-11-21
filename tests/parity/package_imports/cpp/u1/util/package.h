#pragma once

#include <eo/core.h>

#include <string>

using namespace eo;

namespace github_com_conr2d_eo_tests_parity_package_imports_u1_util {

namespace _eo_package = ::github_com_conr2d_eo_tests_parity_package_imports_u1_util;

auto Name() -> std::string;

inline func<> eo_init() {
  co_return;
}

} // namespace github_com_conr2d_eo_tests_parity_package_imports_u1_util
