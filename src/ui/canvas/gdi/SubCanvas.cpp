// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/SubCanvas.hpp"

SubCanvas::SubCanvas(Canvas &canvas,
                     PixelPoint _offset, PixelSize _size) noexcept
{
  /* Share the parent's device context; the implicit HDC conversion
     operator gives us the handle without accessing protected members */
  dc = (HDC)canvas;
  size = _size;

  ::GetViewportOrgEx(dc, &old_viewport);
  ::OffsetViewportOrgEx(dc, _offset.x, _offset.y, nullptr);
}

SubCanvas::~SubCanvas() noexcept
{
  ::SetViewportOrgEx(dc, old_viewport.x, old_viewport.y, nullptr);
  /* Prevent ~Canvas() from releasing the DC -- it belongs to the
     parent canvas */
  dc = nullptr;
}
