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

#ifndef XCSOAR_SCREEN_LAYOUT_HPP
#define XCSOAR_SCREEN_LAYOUT_HPP

#include "Asset.hpp"
#include "Compiler.h"

namespace Layout
{
  extern bool landscape;
  extern bool square;

  /**
   * Fixed-point scaling factor, fractional part is 10 bits (factor
   * 1024).
   */
  extern unsigned scale_1024;
  extern int scale;

  /**
   * Fixed-point scaling factor for on-screen objects which don't grow
   * linearly with the screen resolution.
   */
  extern unsigned small_scale;

  /**
   * Initializes the screen layout information provided by this
   * namespace.
   *
   * @param width the width of the screen in pixels
   * @param height the width of the screen in pixels
   */
  void Initialize(unsigned width, unsigned height);

  /**
   * Is scaling supported by this platform?
   */
  gcc_const
  static inline bool
  ScaleSupported()
  {
    return !is_altair();
  }

  /**
   * Is scaling enabled currently?
   */
  gcc_const
  static inline bool
  ScaleEnabled()
  {
    return ScaleSupported() && scale_1024 > 1024;
  }

  gcc_const
  static inline int
  Scale(int x)
  {
    if (!ScaleSupported())
      return x;

    return (x * (int)scale_1024) >> 10;
  }

  gcc_const
  static inline unsigned
  Scale(unsigned x)
  {
    if (!ScaleSupported())
      return x;

    return (x * scale_1024) >> 10;
  }

  gcc_const
  static inline long
  Scale(long x)
  {
    return Scale((int)x);
  }

  gcc_const
  static inline double
  Scale(double x)
  {
    if (!ScaleSupported())
      return x;

    return x * scale_1024 / 1024.;
  }

  gcc_const
  static inline int
  FastScale(int x)
  {
    if (!ScaleSupported())
      return x;

    return x * scale;
  }

  gcc_const
  static inline int
  SmallScale(int x)
  {
    if (!ScaleSupported())
      return x;

    return (x * (int)small_scale) >> 10;
  }

  gcc_const
  static inline unsigned
  SmallScale(unsigned x)
  {
    if (!ScaleSupported())
      return x;

    return (x * small_scale) >> 10;
  }
}

#define IBLSCALE(x) Layout::Scale(x)

#endif
