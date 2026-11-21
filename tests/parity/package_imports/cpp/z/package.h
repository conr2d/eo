#pragma once

#include <eo/core.h>

#include <cstdint>
#include <string>

#include "github.com/conr2d/eo/tests/parity/package_imports/trace/package.h"

using namespace eo;

namespace github_com_conr2d_eo_tests_parity_package_imports_z {

namespace _eo_package = ::github_com_conr2d_eo_tests_parity_package_imports_z;
namespace trace = ::github_com_conr2d_eo_tests_parity_package_imports_trace;

void _eo_init_0();
func<int64_t> Relay(int64_t n);

inline std::string Z{};

inline func<> eo_init() {
  Z = trace::Mark("z.Z"s);
  _eo_init_0();
  co_return;
}

} // namespace github_com_conr2d_eo_tests_parity_package_imports_z
