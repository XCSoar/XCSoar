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

/*
static inline long lround(double i) {
    return (long)(floor(i+0.5));
}
*/

extern double COSTABLE[4096];
extern double SINETABLE[4096];
extern double INVCOSINETABLE[4096];
extern int ISINETABLE[4096];
extern int ICOSTABLE[4096];

#ifdef __GNUC__
  #define DEG_TO_INT(x) ((unsigned short)(int)((x)*(65536.0/360.0)))>>4
#else
  #define DEG_TO_INT(x) ((unsigned short)((x)*(65536.0/360.0)))>>4
#endif

#define invfastcosine(x) INVCOSINETABLE[DEG_TO_INT(x)]
#define ifastcosine(x) ICOSTABLE[DEG_TO_INT(x)]
#define ifastsine(x) ISINETABLE[DEG_TO_INT(x)]
#define fastcosine(x) COSTABLE[DEG_TO_INT(x)]
#define fastsine(x) SINETABLE[DEG_TO_INT(x)]

#ifdef __cplusplus
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

// Fast trig functions
void InitSineTable(void);

gcc_const
unsigned int isqrt4(unsigned long val);

#ifdef __cplusplus
}
#endif

#endif
