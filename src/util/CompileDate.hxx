// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * Four-digit calendar year from the compiler's `__DATE__` macro (format
 * "MMM DD YYYY").  The value is fixed when each translation unit is compiled.
 */
[[nodiscard]] constexpr int
CompileDateYear() noexcept
{
  return (__DATE__[7] - '0') * 1000 + (__DATE__[8] - '0') * 100 +
         (__DATE__[9] - '0') * 10 + (__DATE__[10] - '0');
}
