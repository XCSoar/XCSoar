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

#include "Screen/Icon.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Canvas.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Texture.hpp"
#include "Screen/OpenGL/Scope.hpp"

#ifdef USE_GLSL
#include "Screen/OpenGL/Shaders.hpp"
#include "Screen/OpenGL/Program.hpp"
#else
#include "Screen/OpenGL/Compatibility.hpp"
#endif
#endif

#include <algorithm>

gcc_const
static unsigned
IconStretchFixed10(unsigned source_dpi)
{
  /* the icons were designed for PDAs at short eye distance; the 3/2
     factor reverses the 2/3 factor applied by Layout::Initialize()
     for small screens */
  return Layout::VptScale(72 * 1024 * 3 / 2) / source_dpi;
}

#ifndef ENABLE_OPENGL

gcc_const
static unsigned
IconStretchInteger(unsigned source_dpi)
{
  return std::max((IconStretchFixed10(source_dpi) + 512) >> 10,
                  1u);
}

#endif

void
MaskedIcon::LoadResource(ResourceId id, ResourceId big_id, bool center)
{
#ifdef ENABLE_OPENGL
  unsigned stretch = 1024;
#endif

  if (Layout::ScaleEnabled()) {
    unsigned source_dpi = 96;
    if (big_id.IsDefined()) {
      id = big_id;
      source_dpi = 192;
    }

#ifdef ENABLE_OPENGL
    stretch = IconStretchFixed10(source_dpi);
    bitmap.Load(id);
    bitmap.EnableInterpolation();
#else
    bitmap.LoadStretch(id, IconStretchInteger(source_dpi));
#endif
  } else
    bitmap.Load(id);

  assert(IsDefined());

  size = bitmap.GetSize();
#ifdef ENABLE_OPENGL
  /* let the GPU stretch on-the-fly */
  size.cx = size.cx * stretch >> 10;
  size.cy = size.cy * stretch >> 10;
#else
  /* left half is mask, right half is icon */
  size.cx /= 2;
#endif

  if (center) {
    origin.x = size.cx / 2;
    origin.y = size.cy / 2;
  } else {
    origin.x = 0;
    origin.y = 0;
  }
}

void
MaskedIcon::Draw(Canvas &canvas, PixelPoint p) const
{
  assert(IsDefined());

  p -= origin;

#ifdef ENABLE_OPENGL
#ifdef USE_GLSL
  OpenGL::texture_shader->Use();
#else
  const GLEnable<GL_TEXTURE_2D> scope;
  OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
#endif

  const ScopeAlphaBlend alpha_blend;

  GLTexture &texture = *bitmap.GetNative();
  texture.Bind();
  texture.Draw(PixelRect(p, size), texture.GetRect());
#else

#ifdef USE_GDI
  /* our icons are monochrome bitmaps, and GDI uses current colors of
     the destination HDC when blitting from a monochrome HDC */
  canvas.SetTextColor(COLOR_BLACK);
  canvas.SetBackgroundColor(COLOR_WHITE);
#endif

  canvas.CopyOr(p.x, p.y, size.cx, size.cy,
                 bitmap, 0, 0);
  canvas.CopyAnd(p.x, p.y, size.cx, size.cy,
                  bitmap, size.cx, 0);
#endif
}

void
MaskedIcon::Draw(Canvas &canvas, const PixelRect &rc, bool inverse) const
{
  const PixelPoint position = rc.CenteredTopLeft(size);

#ifdef ENABLE_OPENGL
#ifdef USE_GLSL
  if (inverse)
    OpenGL::invert_shader->Use();
  else
    OpenGL::texture_shader->Use();
#else
  const GLEnable<GL_TEXTURE_2D> scope;

  if (inverse) {
    OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

    /* invert the texture color */
    OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
    OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_TEXTURE);
    OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_ONE_MINUS_SRC_COLOR);

    /* copy the texture alpha */
    OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
    OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_TEXTURE);
    OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
  } else
    /* simple copy */
    OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
#endif

  const ScopeAlphaBlend alpha_blend;

  GLTexture &texture = *bitmap.GetNative();
  texture.Bind();
  texture.Draw(PixelRect(position, size), texture.GetRect());
#else
  if (inverse) // black background
    canvas.CopyNotOr(position.x, position.y, size.cx, size.cy,
                     bitmap, size.cx, 0);

  else
    canvas.CopyAnd(position.x, position.y, size.cx, size.cy,
                   bitmap, size.cx, 0);
#endif

}
