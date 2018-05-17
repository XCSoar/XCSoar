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

#include "Screen/LargeTextWindow.hpp"

#include <tchar.h>
#include <commctrl.h>
#include <windowsx.h>

void
LargeTextWindow::Create(ContainerWindow &parent, PixelRect rc,
                        const LargeTextWindowStyle style)
{
  Window::Create(&parent, WC_EDIT, nullptr, rc, style);
}

void
LargeTextWindow::SetText(const TCHAR *text)
{
  // Replace \n by \r\r\n to enable usage of line-breaks in edit control
  unsigned size = _tcslen(text);
  TCHAR buffer[size * sizeof(TCHAR) * 3];
  const TCHAR* p2 = text;
  TCHAR* p3 = buffer;
  for (; *p2 != _T('\0'); p2++) {
    if (*p2 == _T('\n')) {
      *p3 = _T('\r');
      p3++;
      *p3 = _T('\r');
      p3++;
      *p3 = _T('\n');
    } else if (*p2 == _T('\r')) {
      continue;
    } else {
      *p3 = *p2;
    }
    p3++;
  }
  *p3 = _T('\0');

  ::SetWindowText(hWnd, buffer);
}

void
LargeTextWindow::ScrollVertically(int delta_lines)
{
  Edit_Scroll(*this, delta_lines, 0);
}
