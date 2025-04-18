// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "../SingleWindow.hpp"
#include "ui/event/windows/Event.hpp"
#include "Resources.hpp"

#include <cassert>

#include <wingdi.h>

namespace UI {

bool
SingleWindow::RegisterClass(HINSTANCE hInstance) noexcept
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
SingleWindow::FilterEvent(const UI::Event &event, Window *allowed) const noexcept
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

} // namespace UI
