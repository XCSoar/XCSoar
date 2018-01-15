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

#include "Screen/OpenGL/Texture.hpp"
#include "Screen/OpenGL/Globals.hpp"
#include "Screen/OpenGL/Features.hpp"
#include "VertexPointer.hpp"
#include "BulkPoint.hpp"
#include "Asset.hpp"
#include "Scope.hpp"
#include "Compiler.h"

#ifdef USE_GLSL
#include "Shaders.hpp"
#include "Program.hpp"

#include <glm/gtc/type_ptr.hpp>
#endif

#ifdef HAVE_OES_DRAW_TEXTURE
#include <GLES/glext.h>
#endif

#ifdef ENABLE_SDL
#include <SDL.h>
#endif

#include <assert.h>

#ifndef NDEBUG
unsigned num_textures;
#endif

gcc_const gcc_unused
static unsigned
NextPowerOfTwo(unsigned i)
{
  unsigned p = 1;
  while (p < i)
    p <<= 1;
  return p;
}

gcc_const
static inline unsigned
ValidateTextureSize(unsigned i)
{
  return OpenGL::texture_non_power_of_two ? i : NextPowerOfTwo(i);
}

gcc_const
static inline PixelSize
ValidateTextureSize(PixelSize size)
{
  return { ValidateTextureSize(size.cx), ValidateTextureSize(size.cy) };
}

/**
 * Load data into the current texture.  Fixes alignment to the next
 * power of two if needed.
 */
static void
LoadTextureAutoAlign(GLint internal_format, PixelSize size,
                     GLenum format, GLenum type, const GLvoid *pixels)
{
  assert(pixels != nullptr);

  PixelSize validated_size = ValidateTextureSize(size);

  if (validated_size == size)
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, size.cx, size.cy, 0,
                 format, type, pixels);
  else {
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format,
                 validated_size.cx, validated_size.cy, 0,
                 format, type, nullptr);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size.cx, size.cy,
                    format, type, pixels);
  }
}

GLTexture::GLTexture(PixelSize _size, bool _flipped)
  :size(_size), allocated_size(ValidateTextureSize(_size)), flipped(_flipped)
{
  Initialise();

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
               allocated_size.cx, allocated_size.cy,
               0, GL_RGB, GetType(), nullptr);
}

GLTexture::GLTexture(GLint internal_format, PixelSize _size,
                     GLenum format, GLenum type, const GLvoid *data,
                     bool _flipped)
  :size(_size), allocated_size(ValidateTextureSize(_size)), flipped(_flipped)
{
  Initialise();
  LoadTextureAutoAlign(internal_format, size, format, type, data);
}

void
GLTexture::ResizeDiscard(PixelSize new_size)
{
  const PixelSize validated_size = ValidateTextureSize(new_size);
  const PixelSize old_size = GetAllocatedSize();

  size = new_size;

  if (validated_size == old_size)
    return;

  allocated_size = validated_size;

  Bind();

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
               validated_size.cx, validated_size.cy,
               0, GL_RGB, GetType(), nullptr);

}

void
GLTexture::Initialise()
{
#ifndef NDEBUG
  ++num_textures;
#endif

  glGenTextures(1, &id);
  Bind();
  Configure();
}

void
GLTexture::Configure()
{
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  GLint filter = IsEmbedded() ? GL_NEAREST : GL_LINEAR;
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
}

void
GLTexture::EnableInterpolation()
{
  if (IsEmbedded()) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  }
}

#ifdef HAVE_OES_DRAW_TEXTURE

inline void
GLTexture::DrawOES(PixelRect dest, PixelRect src) const
{
  const GLint rect[4] = {
    src.left,
    flipped ? src.top : src.bottom,
    GLint(src.GetWidth()),
    flipped ? (GLint)src.GetHeight() : -(GLint)src.GetHeight()
  };

  glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_CROP_RECT_OES, rect);

  /* glDrawTexiOES() circumvents the projection settings, thus we must
     roll our own translation */
  glDrawTexiOES(OpenGL::translate.x + dest.left,
                OpenGL::viewport_size.y - OpenGL::translate.y - dest.bottom,
                0, dest.GetWidth(), dest.GetHeight());
}

#endif

void
GLTexture::Draw(PixelRect dest, PixelRect src) const
{
#ifdef HAVE_OES_DRAW_TEXTURE
  if (OpenGL::oes_draw_texture) {
    DrawOES(dest, src);
    return;
  }
#endif

  const BulkPixelPoint vertices[] = {
    dest.GetTopLeft(),
    dest.GetTopRight(),
    dest.GetBottomLeft(),
    dest.GetBottomRight(),
  };

  const ScopeVertexPointer vp(vertices);

  const PixelSize allocated = GetAllocatedSize();
  GLfloat x0 = (GLfloat)src.left / allocated.cx;
  GLfloat y0 = (GLfloat)src.top / allocated.cy;
  GLfloat x1 = (GLfloat)src.right / allocated.cx;
  GLfloat y1 = (GLfloat)src.bottom / allocated.cy;

  const GLfloat coord[] = {
    x0, flipped ? y1 : y0,
    x1, flipped ? y1 : y0,
    x0, flipped ? y0 : y1,
    x1, flipped ? y0 : y1,
  };

#ifdef USE_GLSL
  glEnableVertexAttribArray(OpenGL::Attribute::TEXCOORD);
  glVertexAttribPointer(OpenGL::Attribute::TEXCOORD, 2, GL_FLOAT, GL_FALSE,
                        0, coord);
#else
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glTexCoordPointer(2, GL_FLOAT, 0, coord);
#endif

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

#ifdef USE_GLSL
  glDisableVertexAttribArray(OpenGL::Attribute::TEXCOORD);
  OpenGL::solid_shader->Use();
#else
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
#endif
}
