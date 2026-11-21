#include "github.com/conr2d/eo/tests/parity/package_imports/a/package.h"

namespace github_com_conr2d_eo_tests_parity_package_imports_a {

auto Shape::Area() const -> int64_t {
  return _eo_call<&Shape::Area>();
}

void _eo_init_0() {
  trace::Add("a.init"s);
}

auto Point::Sum(Point p) -> int64_t {
  return p.X + p.Y;
}

void Point::Scale(ptr<Point> p, int64_t k) {
  p->X *= k;
  p->Y *= k;
}

// Doubled calls a function of z that blocks.
func<int64_t> Doubled(int64_t n) {
  co_return 2 * co_await z::Relay(n);
}

} // namespace github_com_conr2d_eo_tests_parity_package_imports_a
