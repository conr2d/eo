#include "github.com/conr2d/eo/tests/parity/package_files/package.h"

namespace github_com_conr2d_eo_tests_parity_package_files {

void _eo_init_0() {
  fmt::Println("init a.go #1, derived ="s, derived);
}

void _eo_init_1() {
  fmt::Println("init a.go #2, twice ="s, twice(derived));
}

} // namespace github_com_conr2d_eo_tests_parity_package_files
