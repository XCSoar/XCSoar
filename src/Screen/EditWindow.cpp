/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include <commctrl.h>

#if defined(ENABLE_SDL) && !defined(WC_EDIT)
#define WC_EDIT _T("Edit")
#endif

void
EditWindow::set(ContainerWindow &parent, int left, int top,
                unsigned width, unsigned height,
                const EditWindowStyle style) {
  Window::set(&parent, WC_EDIT, NULL,
              left, top, width, height, style);
}

#ifdef ENABLE_SDL
#include "Screen/Canvas.hpp"

void
EditWindow::on_paint(Canvas &canvas)
{
  RECT rc = { 2, 2, canvas.get_width()-4, canvas.get_height()-4 };

  canvas.clear_white();

  canvas.set_text_color(Color::BLACK);
  canvas.background_transparent();
  canvas.formatted_text(&rc, value.c_str(), get_text_style());
}

#endif /* ENABLE_SDL */
