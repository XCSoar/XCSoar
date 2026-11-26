// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StaticArray.hxx"

#include <cstdint>

enum class Unit : uint8_t;

/**
 * The sound resources that say a vertical speed, one word each, to
 * play one after another.  Meters per second and knots are spoken
 * with one decimal, digit by digit: "one point five", "minus zero
 * point five", "one two point three".  Feet per minute are rounded
 * to a hundred: "three hundred", "one thousand two hundred".
 *
 * @param value the vertical speed in the unit #unit
 */
[[gnu::const]]
StaticArray<const char *, 5>
GetSpokenVerticalSpeedClips(double value, Unit unit) noexcept;
