/*
Copyright_License {

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

#ifndef XCSOAR_SCREEN_LAYOUT_HPP
#define XCSOAR_SCREEN_LAYOUT_HPP

#include "Screen/Point.hpp"
#include "Asset.hpp"
#include "Compiler.h"

namespace Layout
{
  extern bool landscape;

  /**
   * Screen size in pixels, the smaller of width and height.
   */
  extern unsigned min_screen_pixels;

  /**
   * Fixed-point scaling factor, fractional part is 10 bits (factor
   * 1024).
   */
  extern unsigned scale_1024;
  extern unsigned scale;

  /**
   * Fixed-point scaling factor for on-screen objects which don't grow
   * linearly with the screen resolution.
   */
  extern unsigned small_scale;

  extern unsigned pen_width_scale;
  extern unsigned fine_pen_width_scale;

  /**
   * Fixed-point scaling factor to convert a point (1/72th inch) to
   * pixels.
   */
  extern unsigned pt_scale;

  /**
   * Fixed-point scaling factor to convert a "virtual point" to
   * pixels.
   *
   * @see VptScale()
   */
  extern unsigned vpt_scale;

  /**
   * Fixed-point scaling factor to convert a font size (in points =
   * 1/72th inch) to pixels.
   */
  extern unsigned font_scale;

  /**
   * Recommended padding from Window boundary to text.
   */
  extern unsigned text_padding;

  extern unsigned minimum_control_height, maximum_control_height;

  extern unsigned hit_radius;

  /**
   * Initializes the screen layout information provided by this
   * namespace.
   *
   * @param screen_size the size of the screen in pixels
   * @param ui_scale the UI scale setting in percent
   * @param custom_dpi user defined DPI setting or 0 for system settings
   */
  void Initialize(PixelSize screen_size, unsigned ui_scale=100,
                  unsigned custom_dpi=0);

  /**
   * Is scaling supported by this platform?
   */
  gcc_const
  static inline bool
  ScaleSupported()
  {
    return true;
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

    return (x * int(scale_1024)) >> 10;
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
    if (!ScaleSupported())
      return x;

    return (x * long(scale_1024)) >> 10;
  }

  gcc_const
  static inline int
  FastScale(int x)
  {
    if (!ScaleSupported())
      return x;

    return x * int(scale);
  }

  gcc_const
  static inline unsigned
  FastScale(unsigned x)
  {
    if (!ScaleSupported())
      return x;

    return x * scale;
  }

  gcc_const
  static inline long
  FastScale(long x)
  {
    if (!ScaleSupported())
      return x;

    return x * (long)scale;
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
  ScalePenWidth(unsigned width)
  {
    if (!ScaleSupported())
      return width;

    return (width * pen_width_scale) >> 10;
  }

  gcc_const
  static inline unsigned
  ScaleFinePenWidth(unsigned width)
  {
    if (!ScaleSupported())
      return width;

    return (width * fine_pen_width_scale) >> 10;
  }

  /**
   * Scale a physical size in points (1/72th inch) to pixels.
   *
   * Use this if you need exact physical dimensions.
   */
  gcc_const
  static inline unsigned
  PtScale(unsigned pt)
  {
    return (pt * pt_scale) >> 10;
  }

  /**
   * Scale a physical size in "virtual points" (nominally 1/72th inch)
   * to pixels.  An additional scaling factor may be applied to
   * consider the reduced viewing distance on small screens.
   *
   * Use this for best readability of on-screen objects.
   */
  gcc_const
  static inline unsigned
  VptScale(unsigned pt)
  {
    return (pt * vpt_scale) >> 10;
  }

  /**
   * Scale a font size in points (1/72th inch) to pixels.  Additional
   * scaling factors may be applied to consider small screens
   * (i.e. viewing distance) and user preference.
   */
  gcc_const
  static inline unsigned
  FontScale(unsigned spt)
  {
    return (spt * font_scale) >> 10;
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
  GetTextPadding()
  {
    if (!ScaleSupported())
      return 2;

    return text_padding;
  }

  /**
   * Returns the minimum height of an dialog control.
   */
  gcc_pure
  static inline unsigned
  GetMinimumControlHeight()
  {
    return minimum_control_height;
  }

  /**
   * Returns the maximum useful height of a dialog control.
   */
  gcc_pure
  static inline unsigned
  GetMaximumControlHeight()
  {
    return maximum_control_height;
  }

  /**
   * Returns the radius (in pixels) of the hit circle around map
   * items.
   */
  gcc_pure
  static inline unsigned
  GetHitRadius()
  {
    if (!HasPointer())
      return 0;

    return hit_radius;
  }
}

#endif
