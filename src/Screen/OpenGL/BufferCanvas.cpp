/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Screen/OpenGL/Scope.hpp"
#include "Screen/OpenGL/Compatibility.hpp"
#include "Texture.hpp"

#include <assert.h>

BufferCanvas::BufferCanvas(const Canvas &canvas,
                           UPixelScalar _width, UPixelScalar _height)
  :Canvas(_width, _height),
   texture(new GLTexture(_width, _height))
{
  assert(canvas.IsDefined());
}

void
BufferCanvas::set(const Canvas &canvas,
                  UPixelScalar _width, UPixelScalar _height)
{
  assert(canvas.IsDefined());
  assert(!active);

  reset();
  texture = new GLTexture(_width, _height);
  Canvas::set(_width, _height);
}

void
BufferCanvas::reset()
{
  assert(!active);

  delete texture;
  texture = NULL;
}

void
BufferCanvas::resize(UPixelScalar _width, UPixelScalar _height)
{
  assert(IsDefined());

  if (_width == width && _height == height)
    return;

  PixelSize new_size { PixelScalar(_width), PixelScalar(_height) };
  texture->ResizeDiscard(new_size);
  Canvas::set(_width, _height);
}

void
BufferCanvas::Begin(Canvas &other)
{
  assert(IsDefined());
  assert(!active);

  x_offset = other.x_offset;
  y_offset = other.y_offset;

  resize(other.get_width(), other.get_height());

  active = true;
}

void
BufferCanvas::Commit(Canvas &other)
{
  assert(IsDefined());
  assert(active);
  assert(x_offset == other.x_offset);
  assert(y_offset == other.y_offset);
  assert(get_width() == other.get_width());
  assert(get_height() == other.get_height());

  PixelRect rc;
  rc.left = 0;
  rc.top = 0;
  rc.right = get_width();
  rc.bottom = get_height();
  CopyToTexture(*texture, rc);

  active = false;
}

void
BufferCanvas::CopyTo(Canvas &other)
{
  assert(IsDefined());
  assert(!active);

  OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

  GLEnable scope(GL_TEXTURE_2D);
  texture->Bind();

  texture->DrawFlipped({ 0, 0, PixelScalar(other.get_width()), PixelScalar(other.get_height()) },
                       { 0, 0, PixelScalar(get_width()), PixelScalar(get_height()) });
}

void
BufferCanvas::surface_created()
{
}

void
BufferCanvas::surface_destroyed()
{
  /* discard the buffer when the Android app is suspended; it needs a
     full redraw to restore it after resuming */

  delete texture;
  texture = NULL;
}
