/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Math/fixed.hpp"
#include "Math/Constants.h"
#include "Compiler.h"

#ifdef FIXED_MATH
extern const int SINETABLE[4096];
extern const fixed::value_t INVCOSINETABLE[4096];
#else
extern const fixed SINETABLE[4096];
extern const fixed INVCOSINETABLE[4096];
#endif

extern const short ISINETABLE[4096];

#ifdef RADIANS
#define INT_ANGLE_MULT fixed(4096.0 / M_2PI)
#else
#define INT_ANGLE_MULT fixed(4096.0 / 360)
#endif

gcc_const
static inline int
NATIVE_TO_INT(fixed x)
{
  return iround(fast_mult(INT_ANGLE_MULT, 8, x, 2)) & 0xfff;
}

gcc_const
static inline int
NATIVE_TO_INT_COS(fixed x)
{
  return (iround(fast_mult(INT_ANGLE_MULT, 8, x, 2)) + 1024) & 0xfff;
}

gcc_const
static inline fixed
invfastcosine(fixed x)
{
#ifdef FIXED_MATH
  return fixed(fixed::internal(), INVCOSINETABLE[NATIVE_TO_INT(x)]);
#else
  return INVCOSINETABLE[NATIVE_TO_INT(x)];
#endif
}

gcc_const
static inline int
ifastsine(fixed x)
{
  return ISINETABLE[NATIVE_TO_INT(x)];
}

gcc_const
static inline int
ifastcosine(fixed x)
{
  return ISINETABLE[NATIVE_TO_INT_COS(x)];
}

gcc_const
static inline fixed
fastsine(fixed x)
{
#ifdef FIXED_MATH
  return fixed(fixed::internal(), SINETABLE[NATIVE_TO_INT(x)]);
#else
  return SINETABLE[NATIVE_TO_INT(x)];
#endif
}

gcc_const
static inline fixed
fastcosine(fixed x)
{
#ifdef FIXED_MATH
  return fixed(fixed::internal(), SINETABLE[NATIVE_TO_INT_COS(x)]);
#else
  return SINETABLE[NATIVE_TO_INT_COS(x)];
#endif
}

#endif
