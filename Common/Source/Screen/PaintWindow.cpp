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

#include "Screen/PaintWindow.hpp"
#include "Screen/PaintCanvas.hpp"

#include <assert.h>

WindowCanvas::WindowCanvas(HWND _wnd, unsigned width, unsigned height)
  :Canvas(::GetDC(_wnd), width, height), wnd(_wnd) {}

WindowCanvas::~WindowCanvas()
{
  reset();
}

void WindowCanvas::set(HWND _wnd, unsigned _width, unsigned _height)
{
  assert(_wnd != NULL);

  reset();
  Canvas::set(GetDC(_wnd), _width, _height);
}

void WindowCanvas::reset()
{
  if (dc != NULL)
    ::ReleaseDC(wnd, dc);
}

void
PaintWindow::set(ContainerWindow *parent,
                 int left, int top, unsigned width, unsigned height,
                 bool center, bool notify, bool show,
                 bool tabstop, bool border)
{
  canvas.reset();

  Window::set(parent, TEXT("STATIC"), TEXT(" "),
              left, top, width, height,
              center, notify, show, tabstop, border);

  if (!canvas.defined())
    canvas.set(hWnd, width, height);
}

void
PaintWindow::set(ContainerWindow &parent,
                 int left, int top, unsigned width, unsigned height,
                 bool center, bool notify, bool show,
                 bool tabstop, bool border)
{
  set(&parent, left, top, width, height,
      center, notify, show, tabstop, border);
}

void
PaintWindow::created(HWND _hWnd)
{
  assert(!canvas.defined());

  Window::created(_hWnd);
  canvas.set(hWnd, 1, 1);
}


void
PaintWindow::reset()
{
  canvas.reset();
  Window::reset();
}

bool
PaintWindow::on_resize(unsigned width, unsigned height)
{
  resize(width, height);
  return true;
}

void
PaintWindow::on_paint(Canvas &canvas)
{
  /* to be implemented by a subclass */
  /* this is not an abstract method yet until the OO transition of all
     PaintWindow users is complete */
}

LRESULT
PaintWindow::on_message(HWND hWnd, UINT message,
                        WPARAM wParam, LPARAM lParam)
{
  switch (message) {
  case WM_ERASEBKGND:
    // we don't need one, we just paint over the top
    return 0;

  case WM_PAINT:
    {
      PaintCanvas canvas(*this, hWnd);
      on_paint(canvas);
    }
    return 0;
  }

  return Window::on_message(hWnd, message, wParam, lParam);
}

bool PaintWindow::register_class(HINSTANCE hInstance, const TCHAR* szWindowClass) {
  // not defined!
  return false;
}
