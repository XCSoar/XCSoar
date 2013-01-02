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

#include "ThermalAssistantLook.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Fonts.hpp"

void
ThermalAssistantLook::Initialise(bool small)
{
  background_color = COLOR_WHITE;
  circle_color = Color(0xB0, 0xB0, 0xB0);
  text_color = Color(0x00, 0x00, 0x00);
  polygon_fill_color = Color(0xCC, 0xCC, 0xFF);
  polygon_border_color = Color(0x00, 0x00, 0xFF);

#ifdef ENABLE_OPENGL
  polygon_brush.Set(polygon_fill_color.WithAlpha(128));
#else /* !OPENGL */
  polygon_brush.Set(polygon_fill_color);
#endif /* !OPENGL */

  UPixelScalar width = Layout::FastScale(small ? 1 : 2);
#ifdef ENABLE_OPENGL
  polygon_pen.Set(width, polygon_border_color.WithAlpha(128));
#else /* !OPENGL */
  polygon_pen.Set(width, polygon_border_color);
#endif /* !OPENGL */
  inner_circle_pen.Set(1, circle_color);
  outer_circle_pen.Set(Pen::DASH, 1, circle_color);
  plane_pen.Set(width, COLOR_BLACK);

  overlay_font.Set(Fonts::GetStandardFontFace(), Layout::FastScale(24));
  circle_label_font.Set(Fonts::GetStandardFontFace(), Layout::FastScale(12));
}
