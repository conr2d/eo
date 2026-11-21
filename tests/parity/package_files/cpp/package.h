#pragma once

#include <eo/fmt.h>

#include <cstdint>
#include <string>
#include <string_view>

using namespace eo;

namespace github_com_conr2d_eo_tests_parity_package_files {

namespace _eo_package = ::github_com_conr2d_eo_tests_parity_package_files;

struct Outer;
struct Inner;

struct Inner {
  static constexpr std::string_view _eo_name = "main.Inner";

  int64_t N{};

  auto operator==(const Inner&) const -> bool = default;
};

// Outer holds Inner, which b.go declares, by value.
struct Outer {
  static constexpr std::string_view _eo_name = "main.Outer";

  Inner In{};
  std::string Tag{};

  auto operator==(const Outer&) const -> bool = default;

  static auto Sum(Outer o) -> int64_t;

  using _eo_methods = eo::methods<eo::method<&Outer::Sum, "Sum">>;
};

void _eo_init_0();
void _eo_init_1();
void _eo_init_2();
auto trace(std::string name, int64_t v) -> int64_t;
void _eo_init_3();
auto twice(int64_t n) -> int64_t;
func<> eo_main();

// derived depends on base, which b.go declares, so base is initialized first.
inline int64_t derived{};
inline int64_t base{};
inline Outer o{};

inline func<> eo_init() {
  base = trace("base"s, 10);
  derived = trace("derived"s, base * 2);
  o = Outer{.In = Inner{.N = derived}, .Tag = "xyz"s};
  _eo_init_0();
  _eo_init_1();
  _eo_init_2();
  _eo_init_3();
  co_return;
}

} // namespace github_com_conr2d_eo_tests_parity_package_files
