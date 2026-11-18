// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once

namespace eo {

// Go's struct{}. Every struct{} is the same type in Go, so it maps to one type rather than to a
// struct per use.
struct empty_struct {
  auto operator==(const empty_struct&) const -> bool = default;
};

} // namespace eo
