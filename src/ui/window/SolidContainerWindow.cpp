// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SolidContainerWindow.hpp"
#include "ui/canvas/Canvas.hpp"

void
SolidContainerWindow::OnPaint(Canvas &canvas) noexcept
{
  canvas.Clear(background_color);

  ContainerWindow::OnPaint(canvas);
}
