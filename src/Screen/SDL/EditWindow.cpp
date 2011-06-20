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

#include "Screen/EditWindow.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Features.hpp"

void
EditWindow::set(ContainerWindow &parent, int left, int top,
                unsigned width, unsigned height,
                const EditWindowStyle style)
{
  read_only = style.is_read_only;

  Window::set(&parent, left, top, width, height, style);
}

void
EditWindow::on_paint(Canvas &canvas)
{
  PixelRect rc = { 2, 2, canvas.get_width()-4, canvas.get_height()-4 };

  if (is_enabled()) {
    if (is_read_only())
      canvas.clear(Color(0xe0, 0xe0, 0xe0));
    else
      canvas.clear_white();
    canvas.set_text_color(COLOR_BLACK);
  } else {
    canvas.clear(COLOR_LIGHT_GRAY);
    canvas.set_text_color(COLOR_DARK_GRAY);
  }

  canvas.background_transparent();

  if (have_clipping() || (get_text_style() & DT_WORDBREAK) != 0)
    canvas.formatted_text(&rc, value.c_str(), get_text_style());
  else
    canvas.TextAutoClipped(rc.left, rc.top, value.c_str());
}
