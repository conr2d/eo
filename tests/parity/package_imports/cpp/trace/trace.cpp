// Package trace records the order in which the packages of the program initialize.
#include "github.com/conr2d/eo/tests/parity/package_imports/trace/package.h"

namespace github_com_conr2d_eo_tests_parity_package_imports_trace {

void Add(std::string e) {
  Events = eo::append(Events, {e});
}

auto Mark(std::string e) -> std::string {
  Add(e);
  return e;
}

void _eo_init_0() {
  Add("trace.init"s);
}

} // namespace github_com_conr2d_eo_tests_parity_package_imports_trace
