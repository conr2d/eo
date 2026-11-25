// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once

#include <string_view>

namespace eo {

// Go's struct{}. Every struct{} is the same type in Go, so it maps to one type rather than to a
// struct per use.
struct empty_struct {
  auto operator==(const empty_struct&) const -> bool = default;
};

// The tag and the embedding of one field of a translated struct, which reflect reports. A struct that
// has a tagged or an embedded field lists one for each field, in order, in _eo_fields.
struct field {
  std::string_view tag;
  bool embedded = false;
};

} // namespace eo
