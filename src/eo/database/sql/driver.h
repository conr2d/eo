// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once

#include <eo/core.h>
#include <eo/errors.h>

#include <string_view>
#include <tuple>

// Go's database/sql/driver: Value and Valuer from Go 1.26, which a type implements so that
// database/sql can store it. Eo has no database/sql, so nothing in Eo calls them.

namespace eo::driver {

struct Value;
struct Valuer;

// Go's driver.Value, a defined type over the empty interface.
struct Value : eo::interface<Value> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "driver.Value";

  using _eo_methods = eo::methods<>;
};

// Go's driver.Valuer.
struct Valuer : eo::interface<Valuer> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "driver.Valuer";

  auto Value() const -> std::tuple<driver::Value, error>;

  using _eo_methods = eo::methods<eo::method<&Valuer::Value, "Value">>;
};

inline auto Valuer::Value() const -> std::tuple<driver::Value, error> {
  return _eo_call<&Valuer::Value>();
}

} // namespace eo::driver
