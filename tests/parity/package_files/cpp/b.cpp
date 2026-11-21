#include "github.com/conr2d/eo/tests/parity/package_files/package.h"

namespace github_com_conr2d_eo_tests_parity_package_files {

void _eo_init_2() {
  fmt::Println("init b.go, base ="s, base);
}

auto trace(std::string name, int64_t v) -> int64_t {
  fmt::Println("var"s, name, "="s, v);
  return v;
}

// Sum is a method of Outer, which a.go declares.
auto Outer::Sum(Outer o) -> int64_t {
  return o.In.N + eo::len(o.Tag);
}

} // namespace github_com_conr2d_eo_tests_parity_package_files
