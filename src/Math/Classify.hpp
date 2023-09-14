// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * A constexpr wrapper for std::isfinite().  This uses the
 * non-standard __builtin_isfinite() function (specific to GCC and
 * clang) because the C++ standard library is not "constexpr".
 */
template<typename T>
constexpr bool
IsFinite(T value) noexcept
{
  return __builtin_isfinite(value);
}
