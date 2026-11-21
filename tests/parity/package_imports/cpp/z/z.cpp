#include "github.com/conr2d/eo/tests/parity/package_imports/z/package.h"

namespace github_com_conr2d_eo_tests_parity_package_imports_z {

void _eo_init_0() {
  trace::Add("z.init"s);
}

// Relay sends n on a new channel from a goroutine and returns what it receives, so it blocks.
func<int64_t> Relay(int64_t n) {
  auto ch = eo::make_chan<int64_t>();
  go([=]() mutable -> func<> { co_await (ch << n); });
  co_return co_await *ch;
}

} // namespace github_com_conr2d_eo_tests_parity_package_imports_z
