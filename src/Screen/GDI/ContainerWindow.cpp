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

#include "Screen/ContainerWindow.hpp"
#include "Screen/ButtonWindow.hpp"
#include "Asset.hpp"

void
ContainerWindow::focus_first_control()
{
  HWND hControl = ::GetNextDlgTabItem(hWnd, hWnd, false);
  if (hControl != NULL)
    ::SetFocus(hControl);
}

void
ContainerWindow::focus_next_control()
{
  HWND hControl = ::GetNextDlgTabItem(hWnd, ::GetFocus(), false);
  if (hControl == NULL)
    return;

  if (is_altair()) { // detect and block wraparound 
    HWND hControl_first = ::GetNextDlgTabItem(hWnd, hWnd, false);
    if (hControl == hControl_first)
      return;
  }

  ::SetFocus(hControl);
}

void
ContainerWindow::focus_previous_control()
{
  HWND hFocus = ::GetFocus();

  if (is_altair()) { // detect and block wraparound 
    HWND hControl_first = ::GetNextDlgTabItem(hWnd, hWnd, false);
    if (hFocus == hControl_first) 
      return;
  }

  HWND hControl = ::GetNextDlgTabItem(hWnd, hFocus, true);
  if (hControl == NULL)
    return;

  ::SetFocus(hControl);
}

LRESULT
ContainerWindow::on_message(HWND hWnd, UINT message,
                            WPARAM wParam, LPARAM lParam)
{
  switch (message) {
  case WM_CTLCOLORSTATIC:
  case WM_CTLCOLORBTN:
    {
      Window *window = Window::get((HWND)lParam);
      if (window == NULL)
        break;

      Canvas canvas((HDC)wParam, 1, 1);
      const Brush *brush = on_color(*window, canvas);
      if (brush == NULL)
        break;

      return (LRESULT)brush->Native();
    }

  case WM_DRAWITEM:
    /* forward WM_DRAWITEM to the child window who sent this
       message */
    {
      const DRAWITEMSTRUCT *di = (const DRAWITEMSTRUCT *)lParam;

      Window *window = Window::get(di->hwndItem);
      if (window == NULL)
        break;

      Canvas canvas(di->hDC, di->rcItem.right - di->rcItem.left,
                    di->rcItem.bottom - di->rcItem.top);
      window->on_paint(canvas);
      return TRUE;
    }

  case WM_COMMAND:
    if (wParam == MAKEWPARAM(BaseButtonWindow::COMMAND_BOUNCE_ID, BN_CLICKED)) {
      /* forward this message to ButtonWindow::on_clicked() */
      BaseButtonWindow *window = (BaseButtonWindow *)Window::get((HWND)lParam);
      if (window != NULL && window->on_clicked())
        return true;
    }
    break;
  };

  return PaintWindow::on_message(hWnd, message, wParam, lParam);
}

