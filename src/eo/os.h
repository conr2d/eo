// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/core.h>

namespace eo::os {

auto ExpandEnv(const std::string& s) -> std::string;

// Go's os.Exit: ends the process at once with the given status, from any goroutine. Deferred calls
// do not run.
[[noreturn]] void Exit(int code);

} // namespace eo::os
