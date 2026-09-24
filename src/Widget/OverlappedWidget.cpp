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
