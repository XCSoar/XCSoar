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
#include <math.h>

#ifdef __cplusplus

#include "Math/fixed.hpp"

// =================================================================================
// Real2Int
// =================================================================================
// 2^36 * 1.5,  (52-_shiftamt=36) uses limited precisicion to floor
// 16.16 fixed point representation,

gcc_const static inline int
Real2Int(double val)
{
  #if defined(WINDOWSPC) && defined(BROKEN)
  // JMW this is broken?
  val += 68719476736.0*1.5;
  return *((long*)&val) >> 16;
  #else
  return (int)val;
  #endif
}

gcc_const static inline int
iround(double i)
{
  return Real2Int(floor(i + 0.5));
}

extern const fixed COSTABLE[4096];
extern const fixed SINETABLE[4096];
extern const fixed INVCOSINETABLE[4096];
extern const int ISINETABLE[4096];
extern const int ICOSTABLE[4096];

gcc_const
static inline int
DEG_TO_INT(fixed x)
{
  return ((unsigned short)(x * fixed(65536.0 / 360.0))) >> 4;
}

gcc_const
static inline fixed
invfastcosine(fixed x)
{
  return INVCOSINETABLE[DEG_TO_INT(x)];
}

gcc_const
static inline int
ifastsine(fixed x)
{
  return ISINETABLE[DEG_TO_INT(x)];
}

gcc_const
static inline int
ifastcosine(fixed x)
{
  return ICOSTABLE[DEG_TO_INT(x)];
}

gcc_const
static inline fixed
fastsine(fixed x)
{
  return SINETABLE[DEG_TO_INT(x)];
}

gcc_const
static inline fixed
fastcosine(fixed x)
{
  return COSTABLE[DEG_TO_INT(x)];
}

gcc_const static inline int
iround(const fixed &x)
{
  return floor(x+fixed_half).as_int();
}

gcc_const static inline int
Real2Int(const fixed& val)
{
  return val.as_int();
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
