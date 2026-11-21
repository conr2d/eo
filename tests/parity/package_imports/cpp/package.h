#pragma once

#include <eo/fmt.h>

#include <cstdint>
#include <string>
#include <string_view>

#include "github.com/conr2d/eo/tests/parity/package_imports/a/package.h"
#include "github.com/conr2d/eo/tests/parity/package_imports/m/package.h"
#include "github.com/conr2d/eo/tests/parity/package_imports/trace/package.h"
#include "github.com/conr2d/eo/tests/parity/package_imports/u1/util/package.h"
#include "github.com/conr2d/eo/tests/parity/package_imports/u2/util/package.h"

using namespace eo;

namespace github_com_conr2d_eo_tests_parity_package_imports {

namespace _eo_package = ::github_com_conr2d_eo_tests_parity_package_imports;
namespace a = ::github_com_conr2d_eo_tests_parity_package_imports_a;
namespace trace = ::github_com_conr2d_eo_tests_parity_package_imports_trace;

struct Holder;
struct Square;

// Holder embeds a type of another package. util here is not the util of main.go.
struct Holder {
  static constexpr std::string_view _eo_name = "main.Holder";

  a::Point Point{};
  ::github_com_conr2d_eo_tests_parity_package_imports_u2_util::Box Box{};
  std::string trace{};

  auto operator==(const Holder&) const -> bool = default;

  static void Record(Holder h);
  static void Scale(ptr<Holder> _eo_receiver, int64_t k);
  static auto Sum(Holder _eo_receiver) -> int64_t;

  using _eo_methods = eo::methods<eo::method<&Holder::Record, "Record">,
    eo::method<&Holder::Scale, "Scale">,
    eo::method<&Holder::Sum, "Sum">>;
};

struct Square {
  static constexpr std::string_view _eo_name = "main.Square";

  int64_t Side{};

  auto operator==(const Square&) const -> bool = default;

  static auto Area(Square s) -> int64_t;

  using _eo_methods = eo::methods<eo::method<&Square::Area, "Area">>;
};

func<> eo_main();
auto box(std::string label) -> ::github_com_conr2d_eo_tests_parity_package_imports_u2_util::Box;
auto other() -> std::string;

inline func<> eo_init() {
  co_return;
}

} // namespace github_com_conr2d_eo_tests_parity_package_imports
