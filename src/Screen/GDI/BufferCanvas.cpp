/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

BufferCanvas::BufferCanvas(const Canvas &canvas, unsigned _width, unsigned _height)
  :VirtualCanvas(canvas, _width, _height)
{
  bitmap = ::CreateCompatibleBitmap(canvas, width, height);
  ::SelectObject(dc, bitmap);
}

BufferCanvas::~BufferCanvas()
{
  reset();
}

void BufferCanvas::set(const Canvas &canvas, unsigned _width, unsigned _height)
{
  assert(canvas.defined());

  reset();
  VirtualCanvas::set(canvas, _width, _height);
  bitmap = ::CreateCompatibleBitmap(canvas, width, height);
  ::SelectObject(dc, bitmap);
}

void
BufferCanvas::set(const Canvas &canvas)
{
  set(canvas, canvas.get_width(), canvas.get_height());
}

void BufferCanvas::reset()
{
  VirtualCanvas::reset();
  if (bitmap != NULL)
    ::DeleteObject(bitmap);
}

void BufferCanvas::resize(unsigned _width, unsigned _height)
{
  assert(dc != NULL);

  if (_width == width && _height == height)
    return;

  ::DeleteObject(bitmap);
  Canvas::resize(_width, _height);
  bitmap = ::CreateCompatibleBitmap(dc, width, height);
  ::SelectObject(dc, bitmap);
}
