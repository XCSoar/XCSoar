/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>

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

#include "Screen/ContainerWindow.hpp"

Brush *
ContainerWindow::on_color(Window &window, Canvas &canvas)
{
  return NULL;
}

#ifdef ENABLE_SDL

bool
ContainerWindow::on_destroy()
{
  for (std::list<Window*>::const_iterator i = children.begin();
       i != children.end(); ++i)
    (*i)->clear_parent();

  PaintWindow::on_destroy();
  return true;
}

void
ContainerWindow::on_paint(Canvas &canvas)
{
  for (std::list<Window*>::const_iterator i = children.begin();
       i != children.end(); ++i) {
    Window &child = **i;
    child.paint();

    canvas.copy(child.get_left(), child.get_top(),
                child.get_canvas().get_width(),
                child.get_canvas().get_height(),
                child.get_canvas(), 0, 0);
  }
}


Window *
ContainerWindow::child_at(int x, int y)
{
  for (std::list<Window*>::const_reverse_iterator i = children.rbegin();
       i != children.rend(); ++i) {
    Window &child = **i;

    if (x >= child.get_left() && x < child.get_right() &&
        y >= child.get_top() && y < child.get_bottom())
      return &child;
  }

  return NULL;
}

#else /* !ENABLE_SDL */

LRESULT
ContainerWindow::on_message(HWND hWnd, UINT message,
                            WPARAM wParam, LPARAM lParam)
{
  switch (message) {
  case WM_CTLCOLORSTATIC:
    Window *window = Window::get((HWND)lParam);
    if (window == NULL)
      break;

    Canvas canvas((HDC)wParam, 1, 1);
    Brush *brush = on_color(*window, canvas);
    if (brush == NULL)
      break;

    return (LRESULT)brush->native();
  };

  return PaintWindow::on_message(hWnd, message, wParam, lParam);
}

#endif /* !ENABLE_SDL */
