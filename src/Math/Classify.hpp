// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <concepts>

#if defined(__clang__) && __clang_major__ >= 18
/* suppress clang 18 warning due to -ffast-math; we want to leave the
   __builtin_isfinite() check, because it might be helpful with some
   compilers or compiler versions, and it's only used as sanity
   check */
#pragma GCC diagnostic ignored "-Wnan-infinity-disabled"
#endif

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
