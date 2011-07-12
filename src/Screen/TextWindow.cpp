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

#include "Screen/TextWindow.hpp"

#ifdef USE_GDI
#include <commctrl.h>
#endif

void
TextWindow::set(ContainerWindow &parent, const TCHAR *_text,
                int left, int top, unsigned width, unsigned height,
                const TextWindowStyle style)
{
#ifndef USE_GDI
  if (_text != NULL)
    text = _text;
  else
    text.clear();
#endif

  Window::set(&parent,
#ifdef USE_GDI
              WC_STATIC, _text,
#endif
              left, top, width, height,
              style);
}

#ifndef USE_GDI
#include "Screen/Canvas.hpp"

void
TextWindow::on_paint(Canvas &canvas)
{
#ifndef ENABLE_OPENGL
  canvas.clear_white();
#endif

  if (!text.empty()) {
    canvas.set_text_color(COLOR_BLACK);
    canvas.background_transparent();
    canvas.text(1, 1, text.c_str());
  }
}

#endif /* !USE_GDI */
