// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "../LargeTextWindow.hpp"

#include <memory>

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

  const std::unique_ptr<TCHAR[]> allocation(new TCHAR[size * 3 + 1]);
  TCHAR *buffer = allocation.get();

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
