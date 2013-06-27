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

#ifndef XCSOAR_SCREEN_WINDOW_CANVAS_HXX
#define XCSOAR_SCREEN_WINDOW_CANVAS_HXX

#include "Screen/Window.hpp"
#include "Screen/Canvas.hpp"

class PaintWindow;

/**
 * A #Canvas implementation which allows you to draw directly into a
 * #PaintWindow, outside of the PaintWindow::OnPaint().
 */
class WindowCanvas : public Canvas {
#ifdef USE_MEMORY_CANVAS
public:
  explicit WindowCanvas(Window &window) {
    buffer.width = window.GetWidth();
    buffer.height = window.GetHeight();
  }

#else /* !USE_MEMORY_CANVAS */

protected:
  HWND wnd;

public:
  explicit WindowCanvas(PaintWindow &window);

  ~WindowCanvas() {
    ::ReleaseDC(wnd, dc);
  }
#endif /* !USE_MEMORY_CANVAS */
};

#endif
