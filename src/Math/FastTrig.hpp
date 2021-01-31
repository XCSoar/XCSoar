/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef XCSOAR_MATH_FAST_TRIG_HPP
#define XCSOAR_MATH_FAST_TRIG_HPP

#include "Constants.hpp"
#include "util/Compiler.h"

static constexpr unsigned INT_ANGLE_RANGE = 4096;
static constexpr unsigned INT_ANGLE_MASK = INT_ANGLE_RANGE - 1;
static constexpr unsigned INT_QUARTER_CIRCLE = INT_ANGLE_RANGE / 4;

static constexpr double INT_ANGLE_MULT = INT_ANGLE_RANGE / M_2PI;

extern const double SINETABLE[INT_ANGLE_RANGE];
extern const double INVCOSINETABLE[INT_ANGLE_RANGE];

extern const short ISINETABLE[INT_ANGLE_RANGE];

constexpr unsigned
NormalizeIntAngle(unsigned angle)
{
  return angle & INT_ANGLE_MASK;
}

constexpr unsigned
IntAngleForCos(unsigned angle)
{
  return NormalizeIntAngle(angle + INT_QUARTER_CIRCLE);
}

constexpr unsigned
UnsafeRadiansToIntAngle(double radians)
{
  /* add INT_ANGLE_RANGE to ensure that the result is not negative
     (assuming the parameter is "sane"); the caller will normalise the
     result using NormalizeIntAngle() */
  /* add 0.5 for rounding; by definition, casting to integer rounds
     towards zero, and adding 0.5 will apply correct rounding for
     non-negative values */
  return unsigned(radians * INT_ANGLE_MULT + (10 * INT_ANGLE_RANGE + 0.5));
}

constexpr unsigned
NATIVE_TO_INT(double x)
{
  return NormalizeIntAngle(UnsafeRadiansToIntAngle(x));
}

constexpr unsigned
NATIVE_TO_INT_COS(double x)
{
  return IntAngleForCos(UnsafeRadiansToIntAngle(x));
}

constexpr double
IntAngleToRadians(unsigned angle)
{
  return angle / INT_ANGLE_MULT;
}

gcc_const
static inline double
invfastcosine(double x)
{
  return INVCOSINETABLE[NATIVE_TO_INT(x)];
}

gcc_const
static inline int
ifastsine(double x)
{
  return ISINETABLE[NATIVE_TO_INT(x)];
}

gcc_const
static inline int
ifastcosine(double x)
{
  return ISINETABLE[NATIVE_TO_INT_COS(x)];
}

gcc_const
static inline double
fastsine(double x)
{
  return SINETABLE[NATIVE_TO_INT(x)];
}

gcc_const
static inline double
fastcosine(double x)
{
  return SINETABLE[NATIVE_TO_INT_COS(x)];
}

#endif
