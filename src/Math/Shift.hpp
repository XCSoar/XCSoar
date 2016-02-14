/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
 */

#ifndef XCSOAR_MATH_SHIFT_HPP
#define XCSOAR_MATH_SHIFT_HPP

/**
 * Shift the given integral value to the right, rounding towards the
 * nearest integral value.
 */
template<typename T>
static inline constexpr T
RoundingRightShift(T value, unsigned bits)
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

#endif
