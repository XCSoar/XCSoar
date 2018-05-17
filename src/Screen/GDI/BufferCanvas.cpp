/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Screen/BufferCanvas.hpp"

#include <assert.h>

BufferCanvas::BufferCanvas(const Canvas &canvas, PixelSize new_size)
  :VirtualCanvas(canvas, new_size)
{
  bitmap = ::CreateCompatibleBitmap(canvas, new_size.cx, new_size.cy);
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
  bitmap = ::CreateCompatibleBitmap(canvas, new_size.cx, new_size.cy);
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
  bitmap = ::CreateCompatibleBitmap(dc, new_size.cx, new_size.cy);
  ::SelectObject(dc, bitmap);

  /* delete the temporary HBITMAP */
#ifndef NDEBUG
  success =
#endif
    ::DeleteObject(tmp);
  assert(success);
}
