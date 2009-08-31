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

#include "Screen/Window.hpp"
#include "Screen/ContainerWindow.hpp"
#include "Interface.hpp"

#ifdef PNA
#include "Appearance.hpp" // for GlobalModelType
#include "Asset.hpp" // for MODELTYPE_*
#endif

#include <assert.h>

void
Window::set(ContainerWindow *parent, LPCTSTR cls, LPCTSTR text,
            unsigned left, unsigned top,
            unsigned width, unsigned height,
            DWORD style, DWORD ex_style)
{
  hWnd = ::CreateWindowEx(ex_style, cls, text, style,
                          left, top, width, height,
                          parent != NULL ? parent->hWnd : NULL,
                          NULL, hInst, this);
}

void
Window::set(ContainerWindow *parent, LPCTSTR cls, LPCTSTR text,
            unsigned left, unsigned top,
            unsigned width, unsigned height,
            bool center, bool notify, bool show,
            bool tabstop, bool border)
{
  DWORD ex_style = 0;
  DWORD style = WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

  if (parent == NULL)
    style |= WS_POPUP;
  else
    style |= WS_CHILD;

  if (show)
    style |= WS_VISIBLE;

  if (center)
    style |= SS_CENTER;

  if (notify)
    style |= SS_NOTIFY;

  if (tabstop)
    style |= WS_TABSTOP;

  if (border) {
    style |= WS_BORDER;

#ifdef PNA // VENTA3 FIX  better borders
    if (GlobalModelType == MODELTYPE_PNA_HP31X) {
      ex_style |= WS_EX_CLIENTEDGE;
      style |= WS_THICKFRAME;
    }
#endif
  }

  set(parent, cls, text, left, top, width, height, style, ex_style);
}

void
Window::created(HWND _hWnd)
{
  assert(hWnd == NULL);
  hWnd = _hWnd;
}

void
Window::reset()
{
  DestroyWindow(hWnd);
}

bool
Window::on_create()
{
  return false;
}

bool
Window::on_destroy()
{
  return false;
}

bool
Window::on_close()
{
  return false;
}

bool
Window::on_resize(unsigned width, unsigned height)
{
  return false;
}

bool
Window::on_mouse_down(unsigned x, unsigned y)
{
  return false;
}

bool
Window::on_mouse_up(unsigned x, unsigned y)
{
  return false;
}

bool
Window::on_mouse_double(unsigned x, unsigned y)
{
  return false;
}

bool
Window::on_key_down(unsigned key_code)
{
  return false;
}

bool
Window::on_key_up(unsigned key_code)
{
  return false;
}

bool
Window::on_command(HWND hWnd, unsigned id, unsigned code)
{
}

LRESULT
Window::on_message(HWND _hWnd, UINT message,
                       WPARAM wParam, LPARAM lParam)
{
  switch (message) {
  case WM_CREATE:
    if (on_create()) return true;
    break;

  case WM_DESTROY:
    if (on_destroy()) return true;
    break;

  case WM_CLOSE:
    if (on_close())
      /* true returned: message was handled */
      return 0;
    break;

  case WM_SIZE:
    if (on_resize(LOWORD(lParam), HIWORD(lParam))) return true;
    break;

  case WM_LBUTTONDOWN:
    if (on_mouse_down(LOWORD(lParam), HIWORD(lParam))) return true;
    break;

  case WM_LBUTTONUP:
    if (on_mouse_up(LOWORD(lParam), HIWORD(lParam))) return true;
    break;

  case WM_LBUTTONDBLCLK:
    if (on_mouse_double(LOWORD(lParam), HIWORD(lParam))) return true;
    break;

  case WM_KEYDOWN:
    if (on_key_down(wParam)) return true;
    break;

  case WM_KEYUP:
    if (on_key_up(wParam)) return true;
    break;

  case WM_COMMAND:
    if (on_command((HWND)lParam, LOWORD(wParam), HIWORD(wParam))) {
      /* true returned: message was handled */
      return 0;
    }
    break;
  }

  return ::DefWindowProc(_hWnd, message, wParam, lParam);
}

LRESULT CALLBACK
Window::WndProc(HWND _hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  if (message == WM_GETMINMAXINFO)
    /* WM_GETMINMAXINFO is called before WM_CREATE, and we havn't set
       a Window pointer yet - let DefWindowProc() handle it */
    return ::DefWindowProc(_hWnd, message, wParam, lParam);

  Window *window;
  if (message == WM_NCCREATE) {
    LPCREATESTRUCT cs = (LPCREATESTRUCT)lParam;

    window = (Window *)cs->lpCreateParams;
    window->created(_hWnd);
    window->set_userdata(window);
  } else
    window = (Window *)get_userdata_pointer(_hWnd);

  return window->on_message(_hWnd, message, wParam, lParam);
}
