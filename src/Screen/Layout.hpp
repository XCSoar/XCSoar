/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef XCSOAR_SCREEN_LAYOUT_HPP
#define XCSOAR_SCREEN_LAYOUT_HPP

#include "Math/fixed.hpp"
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

  /**
   * Scale a vertical dimension value according to the aspect ratio of
   * the display, to work around non-square pixels.  An ellipsis with
   * the pixel width "x" and the pixel height "ScaleY(x)" shall be a
   * circle.
   */
  gcc_const
  static inline int
  ScaleY(int y)
  {
    return y;
  }

  gcc_const
  static inline unsigned
  ScaleY(unsigned y)
  {
    return y;
  }

  gcc_const
  static inline fixed
  ScaleY(fixed y)
  {
    return y;
  }
}

#endif
