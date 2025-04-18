// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "OverlappedWidget.hpp"
#include "ui/window/Window.hpp"

void
OverlappedWidget::Raise() noexcept
{
  assert(IsDefined());
  assert(GetWindow().IsVisible());

  GetWindow().BringToTop();
}

#ifdef USE_WINUSER

void
OverlappedWidget::Hide() noexcept
{
  assert(IsDefined());
  assert(GetWindow().IsVisible());

  /* WindowWidget::Hide() uses Window::FastHide() to reduce overhead,
     but that doesn't work well for overlapped windows, because hiding
     an overlapped Widget must redraw the area behind it */
  GetWindow().Hide();
}

#endif
