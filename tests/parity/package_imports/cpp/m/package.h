#pragma once

#include <eo/core.h>

#include "github.com/conr2d/eo/tests/parity/package_imports/trace/package.h"

using namespace eo;

namespace github_com_conr2d_eo_tests_parity_package_imports_m {

namespace _eo_package = ::github_com_conr2d_eo_tests_parity_package_imports_m;
namespace trace = ::github_com_conr2d_eo_tests_parity_package_imports_trace;

void _eo_init_0();

inline func<> eo_init() {
  _eo_init_0();
  co_return;
}

} // namespace github_com_conr2d_eo_tests_parity_package_imports_m
