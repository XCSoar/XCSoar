// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <concepts>

/**
 * A constexpr wrapper for std::isfinite().  This uses the
 * non-standard __builtin_isfinite() function (specific to GCC and
 * clang) because the C++ standard library is not "constexpr".
 */
constexpr bool
IsFinite(std::floating_point auto value) noexcept
{
  return __builtin_isfinite(value);
}
