// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once

#include <scope>

#define EO_CONCAT_IMPL(a, b) a##b
#define EO_CONCAT(a, b) EO_CONCAT_IMPL(a, b)
#define eo_defer(...) auto EO_CONCAT(_eo_defer_, __COUNTER__) = std::scope_exit(__VA_ARGS__)
