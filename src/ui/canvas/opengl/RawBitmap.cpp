/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "../RawBitmap.hpp"
#include "Canvas.hpp"
#include "Texture.hpp"
#include "Scope.hpp"
#include "Shaders.hpp"
#include "Program.hpp"

#include <cassert>

/**
 * Returns minimum width that is greater then the given width and
 * that is acceptable as image width (not all numbers are acceptable)
 */
static inline unsigned
CorrectedWidth(unsigned nWidth)
{
  return ((nWidth + 3) / 4) * 4;
}

RawBitmap::RawBitmap(unsigned nWidth, unsigned nHeight)
  :width(nWidth), height(nHeight),
   corrected_width(CorrectedWidth(nWidth)),
   buffer(new RawColor[corrected_width * height]),
   texture(new GLTexture(PixelSize(corrected_width, nHeight)))
{
  assert(nWidth > 0);
  assert(nHeight > 0);

  texture->EnableInterpolation();

  AddSurfaceListener(*this);
}

RawBitmap::~RawBitmap()
{
  RemoveSurfaceListener(*this);

  delete texture;
}

void
RawBitmap::SurfaceCreated()
{
  if (texture == nullptr) {
    texture = new GLTexture(PixelSize(corrected_width, height));
    texture->EnableInterpolation();
  }
}

void
RawBitmap::SurfaceDestroyed()
{
  delete texture;
  texture = nullptr;

  dirty = true;
}

GLTexture &
RawBitmap::BindAndGetTexture() const
{
  texture->Bind();

  if (dirty) {
#ifdef HAVE_GLES
    /* 16 bit 5/6/5 on Android */
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, corrected_width, this->height,
                    GL_RGB, GL_UNSIGNED_SHORT_5_6_5, GetBuffer());
#else
    /* 32 bit R/G/B/A on full OpenGL */
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, corrected_width, this->height,
                    GL_BGRA, GL_UNSIGNED_BYTE, GetBuffer());
#endif

    dirty = false;
  }

  return *texture;
}

void
RawBitmap::StretchTo(PixelSize src_size,
                     Canvas &dest_canvas, PixelSize dest_size,
                     gcc_unused bool transparent_white) const
{
  GLTexture &texture = BindAndGetTexture();

  OpenGL::texture_shader->Use();

  texture.Draw(PixelRect{dest_size}, PixelRect{src_size});
}
