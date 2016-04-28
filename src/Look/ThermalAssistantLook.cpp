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

#include "ThermalAssistantLook.hpp"
#include "FontDescription.hpp"
#include "Screen/Layout.hpp"

void
ThermalAssistantLook::Initialise(bool small, bool inverse)
{
  background_color = COLOR_WHITE;
  circle_color = Color(0xB0, 0xB0, 0xB0);
  text_color = Color(0x00, 0x00, 0x00);
  polygon_fill_color = Color(0xCC, 0xCC, 0xFF);
  polygon_border_color = Color(0x00, 0x00, 0xFF);

#ifdef ENABLE_OPENGL
  polygon_brush.Create(polygon_fill_color.WithAlpha(128));
#else /* !OPENGL */
  polygon_brush.Create(polygon_fill_color);
#endif /* !OPENGL */

  unsigned width = Layout::FastScale(small ? 1u : 2u);
#ifdef ENABLE_OPENGL
  polygon_pen.Create(width, polygon_border_color.WithAlpha(128));
#else /* !OPENGL */
  polygon_pen.Create(width, polygon_border_color);
#endif /* !OPENGL */
  inner_circle_pen.Create(1, circle_color);
  outer_circle_pen.Create(Pen::DASH2, 1, circle_color);
  plane_pen.Create(width, inverse ? COLOR_WHITE : COLOR_BLACK);

  overlay_font.Load(FontDescription(Layout::FontScale(22)));
  circle_label_font.Load(FontDescription(Layout::FontScale(10)));
}
