#include "github.com/conr2d/eo/tests/parity/package_imports/package.h"

namespace github_com_conr2d_eo_tests_parity_package_imports {

namespace util = ::github_com_conr2d_eo_tests_parity_package_imports_u1_util;

func<> eo_main() {
  // Go initializes the first package, in import path order, whose imports are initialized:
  // trace, m, z, a. A depth-first walk of the imports would give trace, z, a, m.
  for (auto [_eo_range_key, e] : eo::range(trace::Events)) {
    fmt::Println(e);
  }
  fmt::Println(a::A);
  auto _eo_call_0 = util::Name();
  auto _eo_call_1 = other();
  fmt::Println(_eo_call_0, _eo_call_1);

  auto h = Holder{.Point = a::Point{.X = 1, .Y = 2}, .Box = box("b"s)};
  a::Point::Scale(ref(h.Point), 3);
  fmt::Println(a::Point::Sum(h.Point), h.Point, h.Box.Label);
  fmt::Printf("%T %T\n"s, h.Point, h);

  a::Shape s = Square{.Side = 4};
  auto _eo_call_2 = s.Area();
  auto _eo_call_3 = co_await a::Doubled(21);
  fmt::Println(_eo_call_2, _eo_call_3);

  h.trace = "x"s;
  Holder::Record(h);
  fmt::Println(eo::at(trace::Events, eo::len(trace::Events) - 1));
}

} // namespace github_com_conr2d_eo_tests_parity_package_imports

func<> eo_init() {
  co_await github_com_conr2d_eo_tests_parity_package_imports_trace::eo_init();
  co_await github_com_conr2d_eo_tests_parity_package_imports_m::eo_init();
  co_await github_com_conr2d_eo_tests_parity_package_imports_u1_util::eo_init();
  co_await github_com_conr2d_eo_tests_parity_package_imports_u2_util::eo_init();
  co_await github_com_conr2d_eo_tests_parity_package_imports_z::eo_init();
  co_await github_com_conr2d_eo_tests_parity_package_imports_a::eo_init();
  co_await github_com_conr2d_eo_tests_parity_package_imports::eo_init();
}

func<> eo_main() {
  co_await github_com_conr2d_eo_tests_parity_package_imports::eo_main();
}
