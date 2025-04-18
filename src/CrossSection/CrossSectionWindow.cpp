// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CrossSectionWindow.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scissor.hpp"
#endif

void
CrossSectionWindow::OnPaintBuffer(Canvas &canvas) noexcept
{
  const PixelRect rc = GetClientRect();

#ifdef ENABLE_OPENGL
  const GLCanvasScissor scissor(rc);
#endif
  canvas.DrawFilledRectangle(rc, renderer.inverse? COLOR_BLACK: COLOR_WHITE);
  renderer.Paint(canvas, rc);
}
