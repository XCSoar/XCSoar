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

#include "Texture.hpp"
#include "Globals.hpp"
#include "ui/opengl/Features.hpp"
#include "VertexPointer.hpp"
#include "ui/dim/BulkPoint.hpp"
#include "Asset.hpp"
#include "Scope.hpp"
#include "util/Compiler.h"

#include <glm/gtc/type_ptr.hpp>

#ifdef ENABLE_SDL
#include <SDL.h>
#endif

#include <cassert>

static constexpr unsigned
NextPowerOfTwo(unsigned i) noexcept
{
  unsigned p = 1;
  while (p < i)
    p <<= 1;
  return p;
}

gcc_const
static inline unsigned
ValidateTextureSize(unsigned i) noexcept
{
  return OpenGL::texture_non_power_of_two ? i : NextPowerOfTwo(i);
}

gcc_const
static inline PixelSize
ValidateTextureSize(PixelSize size) noexcept
{
  return { ValidateTextureSize(size.width), ValidateTextureSize(size.height) };
}

/**
 * Load data into the current texture.  Fixes alignment to the next
 * power of two if needed.
 */
static void
LoadTextureAutoAlign(GLint internal_format, PixelSize size,
                     GLenum format, GLenum type, const GLvoid *pixels) noexcept
{
  assert(pixels != nullptr);

  PixelSize validated_size = ValidateTextureSize(size);

  if (validated_size == size)
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, size.width, size.height, 0,
                 format, type, pixels);
  else {
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format,
                 validated_size.width, validated_size.height, 0,
                 format, type, nullptr);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size.width, size.height,
                    format, type, pixels);
  }
}

GLTexture::GLTexture(PixelSize _size, bool _flipped) noexcept
  :size(_size), allocated_size(ValidateTextureSize(_size)), flipped(_flipped)
{
  Initialise();

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
               allocated_size.width, allocated_size.height,
               0, GL_RGB, GetType(), nullptr);
}

GLTexture::GLTexture(GLint internal_format, PixelSize _size,
                     GLenum format, GLenum type, const GLvoid *data,
                     bool _flipped) noexcept
  :size(_size), allocated_size(ValidateTextureSize(_size)), flipped(_flipped)
{
  Initialise();
  LoadTextureAutoAlign(internal_format, size, format, type, data);
}

void
GLTexture::ResizeDiscard(PixelSize new_size) noexcept
{
  const PixelSize validated_size = ValidateTextureSize(new_size);
  const PixelSize old_size = GetAllocatedSize();

  size = new_size;

  if (validated_size == old_size)
    return;

  allocated_size = validated_size;

  Bind();

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
               validated_size.width, validated_size.height,
               0, GL_RGB, GetType(), nullptr);

}

void
GLTexture::Initialise() noexcept
{
  glGenTextures(1, &id);
  Bind();
  Configure();
}

void
GLTexture::Configure() noexcept
{
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  GLint filter = IsEmbedded() ? GL_NEAREST : GL_LINEAR;
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
}

void
GLTexture::EnableInterpolation() noexcept
{
  if (IsEmbedded()) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  }
}

void
GLTexture::Draw(PixelRect dest, PixelRect src) const noexcept
{
  const BulkPixelPoint vertices[] = {
    dest.GetTopLeft(),
    dest.GetTopRight(),
    dest.GetBottomLeft(),
    dest.GetBottomRight(),
  };

  const ScopeVertexPointer vp(vertices);

  const PixelSize allocated = GetAllocatedSize();
  GLfloat x0 = (GLfloat)src.left / allocated.width;
  GLfloat y0 = (GLfloat)src.top / allocated.height;
  GLfloat x1 = (GLfloat)src.right / allocated.width;
  GLfloat y1 = (GLfloat)src.bottom / allocated.height;

  const GLfloat coord[] = {
    x0, flipped ? y1 : y0,
    x1, flipped ? y1 : y0,
    x0, flipped ? y0 : y1,
    x1, flipped ? y0 : y1,
  };

  glEnableVertexAttribArray(OpenGL::Attribute::TEXCOORD);
  glVertexAttribPointer(OpenGL::Attribute::TEXCOORD, 2, GL_FLOAT, GL_FALSE,
                        0, coord);

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  glDisableVertexAttribArray(OpenGL::Attribute::TEXCOORD);
}
