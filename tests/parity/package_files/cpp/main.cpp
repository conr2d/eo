#include "github.com/conr2d/eo/tests/parity/package_files/package.h"

namespace github_com_conr2d_eo_tests_parity_package_files {

void _eo_init_3() {
  fmt::Println("init main.go, o ="s, o);
}

auto twice(int64_t n) -> int64_t {
  return n * 2;
}

func<> eo_main() {
  fmt::Println("main, o.Sum() ="s, Outer::Sum(o));
  co_return;
}

} // namespace github_com_conr2d_eo_tests_parity_package_files

func<> eo_init() {
  co_await github_com_conr2d_eo_tests_parity_package_files::eo_init();
}

func<> eo_main() {
  co_await github_com_conr2d_eo_tests_parity_package_files::eo_main();
}
