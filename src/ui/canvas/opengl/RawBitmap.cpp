// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "../RawBitmap.hpp"
#include "Canvas.hpp"
#include "Texture.hpp"
#include "Scope.hpp"
#include "Shaders.hpp"
#include "Program.hpp"

#include <cassert>

#ifdef USE_RGB565
static constexpr GLint FORMAT = GL_RGB;
static constexpr GLint TYPE = GL_UNSIGNED_SHORT_5_6_5;
#else
static constexpr GLint FORMAT = GL_RGBA;
static constexpr GLint TYPE = GL_UNSIGNED_BYTE;
#endif

RawBitmap::RawBitmap(PixelSize _size) noexcept
  :size(_size),
   buffer(new RawColor[size.width * size.height]),
   texture(new GLTexture(FORMAT, size, FORMAT, TYPE))
{
  assert(size.width > 0);
  assert(size.height > 0);
}

RawBitmap::~RawBitmap() noexcept = default;

GLTexture &
RawBitmap::BindAndGetTexture() const noexcept
{
  texture->Bind();

  if (dirty) {
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size.width, size.height,
                    FORMAT, TYPE, GetBuffer());

    dirty = false;
  }

  return *texture;
}

void
RawBitmap::StretchTo(PixelSize src_size,
                     [[maybe_unused]] Canvas &dest_canvas, PixelSize dest_size,
                     [[maybe_unused]] bool transparent_white) const noexcept
{
  GLTexture &texture = BindAndGetTexture();

  OpenGL::texture_shader->Use();

  texture.Draw(PixelRect{dest_size}, PixelRect{src_size});
}
