/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

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
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "Screen/PaintWindow.hpp"

#ifndef ENABLE_SDL
#include "Screen/PaintCanvas.hpp"
#endif /* !ENABLE_SDL */

#ifdef ENABLE_SDL
#include "Screen/ContainerWindow.hpp"
#endif /* ENABLE_SDL */

#include <assert.h>

#ifndef ENABLE_SDL

WindowCanvas::WindowCanvas(PaintWindow &window)
  :Canvas(::GetDC(window), window.get_width(), window.get_height()),
   wnd(window) {}

#endif /* !ENABLE_SDL */

PaintWindow::~PaintWindow()
{
  reset();
}

bool
PaintWindow::register_class(HINSTANCE hInstance)
{
#ifdef ENABLE_SDL
  return true; // XXX
#else /* !ENABLE_SDL */
  WNDCLASS wc;

  wc.hInstance = hInstance;
  wc.style = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
  wc.lpfnWndProc = Window::WndProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hIcon = (HICON)NULL;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = NULL;
  wc.lpszMenuName = 0;
  wc.lpszClassName = TEXT("PaintWindow");

  return RegisterClass(&wc) != 0;
#endif /* !ENABLE_SDL */
}

bool
PaintWindow::on_erase(Canvas &canvas)
{
  // we don't need one, we just paint over the top
  return true;
}

void
PaintWindow::on_paint(Canvas &canvas)
{
  /* to be implemented by a subclass */
  /* this is not an abstract method yet until the OO transition of all
     PaintWindow users is complete */
}

#ifndef ENABLE_SDL

LRESULT
PaintWindow::on_message(HWND hWnd, UINT message,
                        WPARAM wParam, LPARAM lParam)
{
  switch (message) {
  case WM_ERASEBKGND:
    {
      Canvas canvas((HDC)wParam, get_width(), get_height());
      if (on_erase(canvas))
        return 0;
    }
    break;

  case WM_PAINT:
    {
      PaintCanvas canvas(*this, hWnd);
      on_paint(canvas);
    }
    return 0;
  }

  return Window::on_message(hWnd, message, wParam, lParam);
}

#endif /* !ENABLE_SDL */

bool PaintWindow::register_class(HINSTANCE hInstance, const TCHAR* szWindowClass) {
  // not defined!
  return false;
}
