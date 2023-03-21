// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/BufferCanvas.hpp"

#include <cassert>

BufferCanvas::BufferCanvas(const Canvas &canvas, PixelSize new_size)
  :VirtualCanvas(canvas, new_size)
{
  bitmap = ::CreateCompatibleBitmap(canvas, new_size.width, new_size.height);
  ::SelectObject(dc, bitmap);
}

BufferCanvas::~BufferCanvas()
{
  Destroy();
}

void
BufferCanvas::Create(const Canvas &canvas, PixelSize new_size)
{
  assert(canvas.IsDefined());

  Destroy();
  VirtualCanvas::Create(canvas, new_size);
  bitmap = ::CreateCompatibleBitmap(canvas, new_size.width, new_size.height);
  ::SelectObject(dc, bitmap);
}

void
BufferCanvas::Create(const Canvas &canvas)
{
  Create(canvas, canvas.GetSize());
}

void
BufferCanvas::Destroy()
{
  VirtualCanvas::Destroy();
  if (bitmap != nullptr) {
#ifndef NDEBUG
    bool success =
#endif
      ::DeleteObject(bitmap);
    assert(success);

    bitmap = nullptr;
  }
}

void
BufferCanvas::Resize(PixelSize new_size)
{
  assert(dc != nullptr);

  if (new_size == size)
    return;

  /* create a tiny HBITMAP that will be used to deselect our old
     HBITMAP while remembering the pixel format for us; we can't use
     the "stock" HBITMAP here because it is monochrome, and the
     CreateCompatibleBitmap() below would allocate a monochrome
     HBITMAP */
  HBITMAP tmp = ::CreateCompatibleBitmap(dc, 1, 1);
  ::SelectObject(dc, tmp);

  /* now we can safely deleted our old HBITMAP */
#ifndef NDEBUG
  bool success =
#endif
    ::DeleteObject(bitmap);
  assert(success);

  Canvas::Resize(new_size);
  bitmap = ::CreateCompatibleBitmap(dc, new_size.width, new_size.height);
  ::SelectObject(dc, bitmap);

  /* delete the temporary HBITMAP */
#ifndef NDEBUG
  success =
#endif
    ::DeleteObject(tmp);
  assert(success);
}
