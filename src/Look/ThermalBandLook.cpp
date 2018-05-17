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

#include "ThermalBandLook.hpp"
#include "Screen/Layout.hpp"
#include "Look/Colors.hpp"

void
ThermalBandLook::Initialise(bool _inverse, Color sky_color)
{
  const uint8_t alpha = ALPHA_OVERLAY;

  inverse = _inverse;

  brush_active.Create(ColorWithAlpha(sky_color, alpha));
  brush_inactive.Create(ColorWithAlpha(DarkColor(sky_color), alpha/2));

  pen_active.Create(Layout::ScalePenWidth(1), DarkColor(sky_color));
  pen_inactive.Create(Layout::ScalePenWidth(1), sky_color);

  white_pen.Create(Layout::ScalePenWidth(2), COLOR_WHITE);
  black_pen.Create(Layout::ScalePenWidth(2), COLOR_BLACK);

  working_band_pen.Create(Layout::ScalePenWidth(2), COLOR_GRAY);
}
