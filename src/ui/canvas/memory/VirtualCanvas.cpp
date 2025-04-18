// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/VirtualCanvas.hpp"

#include <cassert>

VirtualCanvas::VirtualCanvas(PixelSize new_size) noexcept
{
  Create(new_size);
}

VirtualCanvas::VirtualCanvas([[maybe_unused]] const Canvas &canvas,
                             PixelSize new_size) noexcept
{
  Create(new_size);
}

void
VirtualCanvas::Create(PixelSize new_size) noexcept
{
  Destroy();

  buffer.Allocate(new_size);
}

void
VirtualCanvas::Create([[maybe_unused]] const Canvas &canvas,
                      PixelSize new_size) noexcept
{
#if defined(ENABLE_OPENGL)
  assert(canvas.IsDefined());
#endif

  Create(new_size);
}

void
VirtualCanvas::Destroy() noexcept
{
  buffer.Free();
}
