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

#include "Screen/ContainerWindow.hpp"
#include "Screen/ButtonWindow.hpp"
#include "Canvas.hpp"
#include "Asset.hpp"

bool
ContainerWindow::FocusFirstControl()
{
  HWND hControl = ::GetNextDlgTabItem(hWnd, nullptr, false);
  if (hControl == NULL)
    return false;

  ::SetFocus(hControl);
  return true;
}

bool
ContainerWindow::FocusNextControl()
{
  HWND hControl = ::GetNextDlgTabItem(hWnd, ::GetFocus(), false);
  if (hControl == NULL)
    return false;

  if (IsAltair()) { // detect and block wraparound 
    HWND hControl_first = ::GetNextDlgTabItem(hWnd, nullptr, false);
    if (hControl == hControl_first)
      return false;
  }

  ::SetFocus(hControl);
  return true;
}

bool
ContainerWindow::FocusPreviousControl()
{
  HWND hFocus = ::GetFocus();

  if (IsAltair()) { // detect and block wraparound 
    HWND hControl_first = ::GetNextDlgTabItem(hWnd, nullptr, false);
    if (hFocus == hControl_first) 
      return false;
  }

  HWND hControl = ::GetNextDlgTabItem(hWnd, hFocus, true);
  if (hControl == NULL)
    return false;

  ::SetFocus(hControl);
  return true;
}

const Brush *
ContainerWindow::OnChildColor(Window &window, Canvas &canvas)
{
  return NULL;
}

LRESULT
ContainerWindow::OnMessage(HWND hWnd, UINT message,
                            WPARAM wParam, LPARAM lParam)
{
  switch (message) {
  case WM_CTLCOLORSTATIC:
  case WM_CTLCOLORBTN:
    {
      Window *window = Window::GetChecked((HWND)lParam);
      if (window == NULL)
        break;

      Canvas canvas((HDC)wParam, {1, 1});
      const Brush *brush = OnChildColor(*window, canvas);
      if (brush == NULL)
        break;

      return (LRESULT)brush->Native();
    }

  case WM_DRAWITEM:
    /* forward WM_DRAWITEM to the child window who sent this
       message */
    {
      const DRAWITEMSTRUCT *di = (const DRAWITEMSTRUCT *)lParam;

      Window *window = Window::GetChecked(di->hwndItem);
      if (window == NULL)
        break;

      Canvas canvas(di->hDC, PixelRect(di->rcItem).GetSize());
      window->OnPaint(canvas);
      return TRUE;
    }

  case WM_COMMAND:
    if (wParam == MAKEWPARAM(BaseButtonWindow::COMMAND_BOUNCE_ID, BN_CLICKED)) {
      /* forward this message to ButtonWindow::OnClicked() */
      BaseButtonWindow *window = (BaseButtonWindow *)Window::GetChecked((HWND)lParam);
      if (window != NULL && window->OnClicked())
        return true;
    }
    break;
  };

  return PaintWindow::OnMessage(hWnd, message, wParam, lParam);
}

