// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * Shift the given integral value to the right, rounding towards the
 * nearest integral value.
 */
template<typename T>
constexpr T
RoundingRightShift(T value, unsigned bits) noexcept
{
  return (value + T(T(1) << (bits - 1))) >> bits;
}

static_assert(RoundingRightShift(0, 8) == 0, "Unit test failed");
static_assert(RoundingRightShift(127, 8) == 0, "Unit test failed");
static_assert(RoundingRightShift(128, 8) == 1, "Unit test failed");
static_assert(RoundingRightShift(255, 8) == 1, "Unit test failed");
static_assert(RoundingRightShift(256, 8) == 1, "Unit test failed");
static_assert(RoundingRightShift(257, 8) == 1, "Unit test failed");
static_assert(RoundingRightShift(383, 8) == 1, "Unit test failed");
static_assert(RoundingRightShift(384, 8) == 2, "Unit test failed");
