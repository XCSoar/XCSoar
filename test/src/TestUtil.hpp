/* Copyright_License {

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

#ifndef XCSOAR_TEST_UTIL_HPP
#define XCSOAR_TEST_UTIL_HPP

#include "Math/Angle.hpp"

extern "C" {
#include "tap.h"
}

int verbose;

static inline bool
is_zero(const fixed value)
{
  return (long)fabs(value * 10000) == 0;
}

static inline bool
is_one(const fixed value)
{
  return is_zero(value - fixed_one);
}

static inline bool
equals(const fixed a, const fixed b)
{
  if (is_zero(a) || is_zero(b))
    return is_zero(a) && is_zero(b);

  return is_one(a / b);
}

#ifdef FIXED_MATH
static inline bool
equals(const fixed a, double b)
{
  return equals(a, fixed(b));
}
#endif

static inline bool
equals(const fixed a, int b)
{
  return equals(a, fixed(b));
}

#ifdef FIXED_MATH
static inline bool
between(double x, double a, double b)
{
  return x >= a && x <= b;
}
#endif

static inline bool
between(fixed x, double a, double b)
{
  return x >= fixed(a) && x <= fixed(b);
}

static inline bool
equals(const Angle a, int b)
{
  return equals(a.value_degrees(), fixed(b));
}

static inline bool
equals(const Angle a, double b)
{
  return equals(a.value_degrees(), fixed(b));
}

#endif
