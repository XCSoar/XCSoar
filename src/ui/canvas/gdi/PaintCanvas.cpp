// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PaintCanvas.hpp"
#include "ui/window/Window.hpp"

PaintCanvas::PaintCanvas(Window &_window)
  :window(_window)
{
  HDC hDC = window.BeginPaint(&ps);
  Create(hDC, window.GetSize());
}

PaintCanvas::~PaintCanvas()
{
  /* Select stock objects into the DC, to unreference the custom
     brushes and pens.  Older Windows CE versions (specifically
     PPC2000/2002) are known to be leaky when the DC is not cleaned
     up.  We don't need to change the bitmap selection, because a
     PaintCanvas should never have a selected bitmap. */
  SelectWhiteBrush();
  SelectBlackPen();
  SelectStockObject(SYSTEM_FONT);

  window.EndPaint(&ps);
}
