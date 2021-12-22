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

#ifndef XCSOAR_SCREEN_LAYOUT_HPP
#define XCSOAR_SCREEN_LAYOUT_HPP

#include "ui/dim/Size.hpp"

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
void
Initialize(PixelSize screen_size, unsigned ui_scale=100,
           unsigned custom_dpi=0) noexcept;

/**
 * Is scaling supported by this platform?
 */
static constexpr bool
ScaleSupported() noexcept
{
  return true;
}

/**
 * Is scaling enabled currently?
 */
[[gnu::const]]
static inline bool
ScaleEnabled() noexcept
{
  return ScaleSupported() && scale_1024 > 1024;
}

[[gnu::const]]
static inline int
Scale(int x) noexcept
{
  if constexpr (!ScaleSupported())
    return x;

  return (x * int(scale_1024)) >> 10;
}

[[gnu::const]]
static inline unsigned
Scale(unsigned x) noexcept
{
  if constexpr (!ScaleSupported())
    return x;

  return (x * scale_1024) >> 10;
}

[[gnu::const]]
static inline long
Scale(long x) noexcept
{
  if constexpr (!ScaleSupported())
    return x;

  return (x * long(scale_1024)) >> 10;
}

[[gnu::const]]
static inline double
Scale(double x) noexcept
{
  if constexpr (!ScaleSupported())
    return x;

  return x * (Layout::scale_1024 / 1024.);
}

[[gnu::const]]
static inline PixelSize
Scale(PixelSize size) noexcept
{
  if constexpr (!ScaleSupported())
    return size;

  return {Scale(size.width), Scale(size.height)};
}

[[gnu::const]]
static inline int
FastScale(int x) noexcept
{
  if constexpr (!ScaleSupported())
    return x;

  return x * int(scale);
}

[[gnu::const]]
static inline unsigned
FastScale(unsigned x) noexcept
{
  if constexpr (!ScaleSupported())
    return x;

  return x * scale;
}

[[gnu::const]]
static inline long
FastScale(long x) noexcept
{
  if constexpr (!ScaleSupported())
    return x;

  return x * (long)scale;
}

[[gnu::const]]
static inline unsigned
ScalePenWidth(unsigned width) noexcept
{
  if constexpr (!ScaleSupported())
    return width;

  return (width * pen_width_scale) >> 10;
}

[[gnu::const]]
static inline unsigned
ScaleFinePenWidth(unsigned width) noexcept
{
  if constexpr (!ScaleSupported())
    return width;

  return (width * fine_pen_width_scale) >> 10;
}

/**
 * Scale a physical size in points (1/72th inch) to pixels.
 *
 * Use this if you need exact physical dimensions.
 */
[[gnu::const]]
static inline unsigned
PtScale(unsigned pt) noexcept
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
[[gnu::const]]
static inline unsigned
VptScale(unsigned pt) noexcept
{
  return (pt * vpt_scale) >> 10;
}

/**
 * Scale a font size in points (1/72th inch) to pixels.  Additional
 * scaling factors may be applied to consider small screens
 * (i.e. viewing distance) and user preference.
 */
[[gnu::const]]
static inline unsigned
FontScale(unsigned spt) noexcept
{
  return (spt * font_scale) >> 10;
}

/**
 * Scale a vertical dimension value according to the aspect ratio of
 * the display, to work around non-square pixels.  An ellipsis with
 * the pixel width "x" and the pixel height "ScaleY(x)" shall be a
 * circle.
 */
[[gnu::const]]
static inline int
ScaleY(int y) noexcept
{
  return y;
}

[[gnu::const]]
static inline unsigned
GetTextPadding() noexcept
{
  if constexpr (!ScaleSupported())
    return 2;

  return text_padding;
}

/**
 * Returns the minimum height of an dialog control.
 */
[[gnu::pure]]
static inline unsigned
GetMinimumControlHeight() noexcept
{
  return minimum_control_height;
}

/**
 * Returns the maximum useful height of a dialog control.
 */
[[gnu::pure]]
static inline unsigned
GetMaximumControlHeight() noexcept
{
  return maximum_control_height;
}

/**
 * Returns the radius (in pixels) of the hit circle around map
 * items.
 */
[[gnu::pure]]
static inline unsigned
GetHitRadius() noexcept
{
  return hit_radius;
}

} // namespace Layout

#endif
