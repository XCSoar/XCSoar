/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#ifndef XCSOAR_MATH_FASTMATH_H
#define XCSOAR_MATH_FASTMATH_H

#include "Compiler.h"
#include "Math/Constants.h"
#include <math.h>

#ifdef __cplusplus

#include "Math/fixed.hpp"

/**
 * Compares a^2 + b^2 against c^2
 * Use this instead of hypot when comparing
 * @return 1 if a^2 + b^2 > c^2,
 *         0 if a^2 + b^2 = c^2,
 *        -1 if a^2 + b^2 < c^2,
 */
gcc_const int compare_squared(int a, int b, int c);

gcc_const static inline int
iround(const fixed &x)
{
  return floor(x+fixed_half).as_int();
}

#ifdef FIXED_MATH
extern const int COSTABLE[4096];
extern const int SINETABLE[4096];
extern const fixed::value_t INVCOSINETABLE[4096];
#else
extern const fixed COSTABLE[4096];
extern const fixed SINETABLE[4096];
extern const fixed INVCOSINETABLE[4096];
#endif

extern const short ISINETABLE[4096];
extern const short ICOSTABLE[4096];

#ifdef RADIANS
#define INT_ANGLE_MULT fixed_constant(4096.0 / M_2PI, 0x28be60db93LL)
#else
#define INT_ANGLE_MULT fixed_constant(4096.0 / 360, 0xb60b60b6LL)
#endif

gcc_const
static inline int
NATIVE_TO_INT(fixed x)
{
  return uround(x * INT_ANGLE_MULT) & 0xfff;
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
  return ICOSTABLE[NATIVE_TO_INT(x)];
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
  return fixed(fixed::internal(), COSTABLE[NATIVE_TO_INT(x)]);
#else
  return COSTABLE[NATIVE_TO_INT(x)];
#endif
}

inline unsigned int
CombinedDivAndMod(unsigned int &lx)
{
  unsigned int ox = lx & 0xff;
  // JMW no need to check max since overflow will result in
  // beyond max dimensions
  lx = lx >> 8;
  return ox;
}

extern "C"
{
#endif

gcc_const
unsigned int isqrt4(unsigned long val);

#ifdef __cplusplus
}
#endif

#endif
