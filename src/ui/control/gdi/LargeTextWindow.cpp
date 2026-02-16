// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "../LargeTextWindow.hpp"

#include <memory>

#include <tchar.h>
#include <commctrl.h>
#include <windowsx.h>

LargeTextWindow::~LargeTextWindow() noexcept
{
  if (background_brush != nullptr)
    ::DeleteObject(background_brush);
}

void
LargeTextWindow::Create(ContainerWindow &parent, PixelRect rc,
                        const LargeTextWindowStyle style)
{
  Window::Create(&parent, WC_EDIT, nullptr, rc, style);

  /* store the Window pointer so the parent can find us when
     reflecting WM_CTLCOLORSTATIC */
  SetUserData(this);
}

void
LargeTextWindow::SetColors(Color _background, Color _text,
                            Color _border) noexcept
{
  background_color = _background;
  text_color = _text;
  border_color = _border;

  if (background_brush != nullptr)
    ::DeleteObject(background_brush);

  background_brush = ::CreateSolidBrush(_background);
  ::InvalidateRect(hWnd, nullptr, true);
}

LRESULT
LargeTextWindow::OnChildColor(HDC hdc) noexcept
{
  if (background_brush == nullptr)
    return 0;

  ::SetTextColor(hdc, text_color);
  ::SetBkColor(hdc, background_color);
  return (LRESULT)background_brush;
}

void
LargeTextWindow::SetText(const char *text)
{
  // Replace \n by \r\r\n to enable usage of line-breaks in edit control
  unsigned size = _tcslen(text);

  const std::unique_ptr<char[]> allocation(new char[size * 3 + 1]);
  char *buffer = allocation.get();

  const char* p2 = text;
  char* p3 = buffer;
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
