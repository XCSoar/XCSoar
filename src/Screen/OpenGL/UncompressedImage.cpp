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

#include "UncompressedImage.hpp"
#include "Texture.hpp"
#include "Screen/Custom/UncompressedImage.hpp"

GLTexture *
ImportTexture(const UncompressedImage &image)
{
  GLint internal_format;
  GLenum format, type;

  switch (image.GetFormat()) {
  case UncompressedImage::Format::GRAY:
    internal_format = GL_LUMINANCE;
    format = GL_LUMINANCE;
    type = GL_UNSIGNED_BYTE;
    break;

  case UncompressedImage::Format::RGB:
    internal_format = GL_RGB;
    format = GL_RGB;
    type = GL_UNSIGNED_BYTE;
    break;

  case UncompressedImage::Format::RGBA:
    internal_format = GL_RGBA;
    format = GL_RGBA;
    type = GL_UNSIGNED_BYTE;
    break;

  case UncompressedImage::Format::INVALID:
    return nullptr;

#ifdef __OPTIMIZE__
  default:
    gcc_unreachable();
#endif
  }

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  return new GLTexture(internal_format, image.GetSize(),
                       format, type, image.GetData(), image.IsFlipped());
}

GLTexture *
ImportAlphaTexture(const UncompressedImage &image)
{
  assert(image.GetFormat() == UncompressedImage::Format::GRAY);

  const GLint internal_format = GL_ALPHA;
  const GLenum format = GL_ALPHA, type = GL_UNSIGNED_BYTE;

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  return new GLTexture(internal_format, image.GetSize(),
                       format, type, image.GetData(), image.IsFlipped());
}
