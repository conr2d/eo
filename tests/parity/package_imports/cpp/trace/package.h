#pragma once

#include <eo/core.h>

#include <string>

using namespace eo;

namespace github_com_conr2d_eo_tests_parity_package_imports_trace {

namespace _eo_package = ::github_com_conr2d_eo_tests_parity_package_imports_trace;

void Add(std::string e);
auto Mark(std::string e) -> std::string;
void _eo_init_0();

inline slice<std::string> Events;

inline func<> eo_init() {
  _eo_init_0();
  co_return;
}

} // namespace github_com_conr2d_eo_tests_parity_package_imports_trace
