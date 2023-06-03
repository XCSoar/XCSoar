// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * Compares a^2 + b^2 against c^2
 * Use this instead of hypot when comparing
 * @return 1 if a^2 + b^2 > c^2,
 *         0 if a^2 + b^2 = c^2,
 *        -1 if a^2 + b^2 < c^2,
 */
constexpr int
compare_squared(int a, int b, int c) noexcept
{
  const unsigned a2b2 = a * a + b * b;
  const unsigned c2 = c * c;
  if (a2b2 > c2)
    return 1;
  if (a2b2 < c2)
    return -1;
  return 0;
}

[[gnu::const]]
unsigned
isqrt4(unsigned val) noexcept;

[[gnu::const]]
static inline unsigned
ihypot(int x, int y) noexcept
{
  return isqrt4(x * x + y * y);
}
