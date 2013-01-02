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

#include "Screen/Layout.hpp"
#include "Hardware/Display.hpp"

#include <algorithm>

using std::min;
using std::max;

namespace Layout
{
  bool landscape = false;
  bool square = false;
  int scale = 1;
  unsigned scale_1024 = 1024;
  unsigned small_scale = 1024;
  unsigned pen_width_scale = 1024;
  UPixelScalar text_padding = 2;
  UPixelScalar minimum_control_height = 22, maximum_control_height = 44;
  UPixelScalar hit_radius = 10;
}

void
Layout::Initialize(unsigned width, unsigned height)
{
  landscape = width > height;
  square = width == height;

  if (!ScaleSupported())
    return;

  const unsigned x_dpi = Display::GetXDPI();

  unsigned minsize = min(width, height);
  // always start w/ shortest dimension
  // square should be shrunk
  scale_1024 = max(1024U, minsize * 1024 / (square ? 320 : 240));
  scale = scale_1024 / 1024;

  small_scale = (scale_1024 - 1024) / 2 + 1024;

  pen_width_scale = std::max(1024u, x_dpi * 1024u / 80u);

  text_padding = Scale(2);

  minimum_control_height = Scale(22);

  if (HasTouchScreen()) {
    /* larger rows for touch screens */
    maximum_control_height = Display::GetYDPI() * 3 / 7;
    if (maximum_control_height < minimum_control_height)
      maximum_control_height = minimum_control_height;
  } else {
    maximum_control_height = minimum_control_height;
  }

  hit_radius = x_dpi / (HasTouchScreen() ? 3 : 12);
}
