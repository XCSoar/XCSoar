/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "Screen/SingleWindow.hpp"
#include "Event/Windows/Event.hpp"
#include "Resources.hpp"

#include <cassert>

bool
SingleWindow::RegisterClass(HINSTANCE hInstance)
{
  WNDCLASS wc;

  wc.hInstance = hInstance;
  wc.style = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
  wc.lpfnWndProc = Window::WndProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hIcon = ::LoadIcon(hInstance, MAKEINTRESOURCE((unsigned)IDI_XCSOAR));
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
  wc.lpszMenuName = 0;
  wc.lpszClassName = class_name;

  return ::RegisterClass(&wc) != 0;
}

bool
SingleWindow::FilterEvent(const Event &event, Window *allowed) const
{
  assert(allowed != nullptr);

  if (event.IsUserInput()) {
    if (allowed->IdentifyDescendant(event.msg.hwnd))
      /* events to the current modal dialog are allowed */
      return true;

    return false;
  } else
    return true;
}
