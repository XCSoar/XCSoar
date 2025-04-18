// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/SubCanvas.hpp"
#include "Globals.hpp"
#include "Shaders.hpp"

SubCanvas::SubCanvas(Canvas &canvas,
                     PixelPoint _offset, PixelSize _size) noexcept
  :relative(_offset)
{
  assert(canvas.offset == OpenGL::translate);
  offset = canvas.offset + _offset;
  size = _size;

  if (relative.x != 0 || relative.y != 0) {
    OpenGL::translate += _offset;
    OpenGL::UpdateShaderTranslate();
  }
}

SubCanvas::~SubCanvas() noexcept
{
  assert(offset == OpenGL::translate);

  if (relative.x != 0 || relative.y != 0) {
    OpenGL::translate -= relative;
    OpenGL::UpdateShaderTranslate();
  }
}
