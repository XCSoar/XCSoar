// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "../PaintWindow.hpp"
#include "ui/canvas/gdi/PaintCanvas.hpp"

bool
PaintWindow::register_class(HINSTANCE hInstance) noexcept
{
  WNDCLASS wc;

  wc.hInstance = hInstance;
  wc.style = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS | CS_PARENTDC;
  wc.lpfnWndProc = Window::WndProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hIcon = (HICON)nullptr;
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
  wc.lpszMenuName = 0;
  wc.lpszClassName = TEXT("PaintWindow");

  return RegisterClass(&wc) != 0;
}

LRESULT
PaintWindow::OnMessage(HWND _hWnd, UINT message,
                       WPARAM wParam, LPARAM lParam) noexcept
{
  if (message == WM_PAINT) {
    PaintCanvas canvas(*this);
    OnPaint(canvas, canvas.get_dirty());
    return 0;
  } else
    return Window::OnMessage(_hWnd, message, wParam, lParam);
}
