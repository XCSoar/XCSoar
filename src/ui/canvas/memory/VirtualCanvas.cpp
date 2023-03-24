// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/VirtualCanvas.hpp"

#include <cassert>

VirtualCanvas::VirtualCanvas(PixelSize new_size)
{
  Create(new_size);
}

VirtualCanvas::VirtualCanvas([[maybe_unused]] const Canvas &canvas, PixelSize new_size)
{
  Create(new_size);
}

void
VirtualCanvas::Create(PixelSize new_size)
{
  Destroy();

  buffer.Allocate(new_size.width, new_size.height);
}

void
VirtualCanvas::Create([[maybe_unused]] const Canvas &canvas, PixelSize new_size)
{
#if defined(ENABLE_OPENGL)
  assert(canvas.IsDefined());
#endif

  Create(new_size);
}

void
VirtualCanvas::Destroy()
{
  buffer.Free();
}
