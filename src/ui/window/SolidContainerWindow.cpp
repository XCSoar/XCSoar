// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SolidContainerWindow.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Renderer/GradientRenderer.hpp"

void
SolidContainerWindow::OnPaint(Canvas &canvas) noexcept
{
  if (has_gradient)
    DrawBandedVerticalGradient(canvas, GetClientRect(),
                               gradient_top_color, background_color);
  else
    canvas.Clear(background_color);

  ContainerWindow::OnPaint(canvas);
}
