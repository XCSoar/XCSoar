// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "HLine.hpp"
#include "Look/DialogLook.hpp"
#include "ui/canvas/Canvas.hpp"

void
HLine::OnPaint(Canvas &canvas) noexcept
{
#ifndef ENABLE_OPENGL
  canvas.Clear(look.background_color);
#endif

  const int width = canvas.GetWidth();
  const unsigned height = canvas.GetHeight();
  const int middle = height / 2;

  canvas.SelectBlackPen();
  canvas.DrawLine({0, middle}, {width, middle});
}
