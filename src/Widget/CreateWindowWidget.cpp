// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CreateWindowWidget.hpp"
#include "ui/window/Window.hpp"

void
CreateWindowWidget::Prepare(ContainerWindow &parent,
                            const PixelRect &rc) noexcept
{
  WindowStyle style;
  style.Hide();
  SetWindow(create(parent, rc, style));
}
