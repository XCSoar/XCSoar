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
                          NULL, hInst, NULL);
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
