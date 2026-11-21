#pragma once

#include <eo/core.h>
#include <eo/interface.h>

#include <cstdint>
#include <string>
#include <string_view>

#include "github.com/conr2d/eo/tests/parity/package_imports/trace/package.h"
#include "github.com/conr2d/eo/tests/parity/package_imports/z/package.h"

using namespace eo;

namespace github_com_conr2d_eo_tests_parity_package_imports_a {

namespace _eo_package = ::github_com_conr2d_eo_tests_parity_package_imports_a;
namespace trace = ::github_com_conr2d_eo_tests_parity_package_imports_trace;
namespace z = ::github_com_conr2d_eo_tests_parity_package_imports_z;

struct Point;
struct Shape;

struct Point {
  static constexpr std::string_view _eo_name = "a.Point";

  int64_t X{}, Y{};

  auto operator==(const Point&) const -> bool = default;

  static auto Sum(Point p) -> int64_t;
  static void Scale(ptr<Point> p, int64_t k);

  using _eo_methods = eo::methods<eo::method<&Point::Sum, "Sum">, eo::method<&Point::Scale, "Scale">>;
};

struct Shape : interface<Shape> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "a.Shape";

  auto Area() const -> int64_t;

  using _eo_methods = eo::methods<eo::method<&Shape::Area, "Area">>;
};

void _eo_init_0();
func<int64_t> Doubled(int64_t n);

inline std::string A{};

inline func<> eo_init() {
  A = trace::Mark("a.A after "s + z::Z);
  _eo_init_0();
  co_return;
}

} // namespace github_com_conr2d_eo_tests_parity_package_imports_a
