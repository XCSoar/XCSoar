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

#include "VarioBarLook.hpp"
#include "Screen/Layout.hpp"

static Color
ColorWithAlpha(const Color &c, uint8_t a)
{
#ifdef ENABLE_OPENGL
  return c.WithAlpha(a);
#else
  return c;
#endif
}


void
VarioBarLook::Initialise(const Font &_font)
{
  const uint8_t alpha = 0xA0;

  brush_sink.Set(ColorWithAlpha(COLOR_RED, alpha));
  brush_sink_avg.Set(ColorWithAlpha(LightColor(COLOR_RED), alpha));
  pen_sink.Set(Layout::ScalePenWidth(1), DarkColor(COLOR_RED));

  brush_climb.Set(ColorWithAlpha(COLOR_GREEN, alpha));
  brush_climb_avg.Set(ColorWithAlpha((LightColor(LightColor(COLOR_GREEN))), alpha));
  pen_climb.Set(Layout::ScalePenWidth(1), DarkColor(COLOR_GREEN));

  brush_mc.Set(ColorWithAlpha(COLOR_GRAY, alpha));
  pen_mc.Set(Layout::ScalePenWidth(1), DarkColor(COLOR_GRAY));

  font = &_font;
}
