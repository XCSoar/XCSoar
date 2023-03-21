// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/VirtualCanvas.hpp"

#include <cassert>

VirtualCanvas::VirtualCanvas(PixelSize new_size)
  :Canvas(::CreateCompatibleDC(nullptr), new_size)
{
}

VirtualCanvas::VirtualCanvas(const Canvas &canvas, PixelSize new_size)
  :Canvas(::CreateCompatibleDC(canvas), new_size)
{
  assert(canvas.IsDefined());
}

void
VirtualCanvas::Create(PixelSize new_size)
{
  Destroy();
  Canvas::Create(CreateCompatibleDC(nullptr), new_size);
}

void
VirtualCanvas::Create(const Canvas &canvas, PixelSize new_size)
{
  assert(canvas.IsDefined());

  Destroy();
  Canvas::Create(CreateCompatibleDC(canvas), new_size);
}

void VirtualCanvas::Destroy()
{
  Canvas::Destroy();

  if (dc != nullptr)
    ::DeleteDC(dc);
}
