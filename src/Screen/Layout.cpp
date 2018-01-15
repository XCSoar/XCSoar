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

#include "Screen/Layout.hpp"
#include "Hardware/DisplayDPI.hpp"
#include "Hardware/DisplaySize.hpp"

#include <algorithm>

namespace Layout
{
  bool landscape = false;
  unsigned min_screen_pixels = 512;
  unsigned scale = 1;
  unsigned scale_1024 = 1024;
  unsigned small_scale = 1024;
  unsigned pen_width_scale = 1024;
  unsigned fine_pen_width_scale = 1024;
  unsigned pt_scale = 1024;
  unsigned vpt_scale = 1024;
  unsigned font_scale = 1024;
  unsigned text_padding = 2;
  unsigned minimum_control_height = 20, maximum_control_height = 44;
  unsigned hit_radius = 10;
}

/**
 * Is the given pixel size smaller than 5 inch?
 */
static constexpr bool
IsSmallScreen(unsigned size, unsigned dpi)
{
  return size < dpi * 5;
}

/**
 * Is the small edge smaller than 5 inch?
 */
static constexpr bool
IsSmallScreen(unsigned width, unsigned height,
              unsigned x_dpi, unsigned y_dpi)
{
  return width < height
    ? IsSmallScreen(width, x_dpi)
    : IsSmallScreen(height, y_dpi);
}

/**
 * Is the small edge smaller than 5 inch?
 */
static constexpr bool
IsSmallScreen(PixelSize size,
              unsigned x_dpi, unsigned y_dpi)
{
  return IsSmallScreen(size.cx, size.cy, x_dpi, y_dpi);
}

void
Layout::Initialize(PixelSize new_size, unsigned ui_scale, unsigned custom_dpi)
{
  const unsigned width = new_size.cx, height = new_size.cy;

  min_screen_pixels = std::min(width, height);
  landscape = width > height;
  const bool square = width == height;

  if (!ScaleSupported())
    return;

  const unsigned x_dpi = Display::GetXDPI(custom_dpi);
  const unsigned y_dpi = Display::GetYDPI(custom_dpi);
  const bool is_small_screen = IsSmallScreen(Display::GetSize(new_size),
                                             x_dpi, y_dpi);

  // always start w/ shortest dimension
  // square should be shrunk
  scale_1024 = std::max(1024U, min_screen_pixels * 1024 / (square ? 320 : 240));
  scale = scale_1024 / 1024;

  small_scale = (scale_1024 - 1024) / 2 + 1024;

  pen_width_scale = std::max(1024u, x_dpi * 1024u / 80u);
  fine_pen_width_scale = std::max(1024u, x_dpi * 1024u / 160u);

  pt_scale = 1024 * y_dpi / 72;

  vpt_scale = pt_scale;
  if (is_small_screen)
    /* small screens (on portable devices) use a smaller font because
       the viewing distance is usually smaller */
    vpt_scale = vpt_scale * 2 / 3;

  font_scale = 1024 * y_dpi * ui_scale / 72 / 100;
  if (is_small_screen)
    /* small screens (on portable devices) use a smaller font because
       the viewing distance is usually smaller */
    font_scale = font_scale * 2 / 3;

  text_padding = VptScale(2);

  minimum_control_height = std::min(FontScale(23),
                                    min_screen_pixels / 12);

  if (HasTouchScreen()) {
    /* larger rows for touch screens */
    maximum_control_height = PtScale(30);
    if (maximum_control_height < minimum_control_height)
      maximum_control_height = minimum_control_height;
  } else {
    maximum_control_height = minimum_control_height;
  }

  hit_radius = PtScale(HasTouchScreen() ? 24 : 6);
}
