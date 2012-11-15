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

#include "Screen/Bitmap.hpp"
#include "Screen/Debug.hpp"
#include "Texture.hpp"
#include "ResourceLoader.hpp"

#include <png.h>

struct PNGCallbackContext {
  const uint8_t *data;
};

static void
PNGReadCallback(png_structp _ctx, png_bytep area, png_size_t size)
{
  PNGCallbackContext &ctx = *(PNGCallbackContext *)png_get_io_ptr(_ctx);

  memcpy(area, ctx.data, size);
  ctx.data += size;
}

static GLTexture *
ImportPNG(unsigned width, unsigned height, int color_type,
          const void *data)
{
  GLint internal_format;
  GLenum format, type;

  switch (color_type) {
  case PNG_COLOR_TYPE_GRAY:
    internal_format = GL_LUMINANCE;
    format = GL_LUMINANCE;
    type = GL_UNSIGNED_BYTE;
    break;

  case PNG_COLOR_TYPE_RGB:
    internal_format = GL_RGB;
    format = GL_RGB;
    type = GL_UNSIGNED_BYTE;
    break;

  default:
    /* not supported */
    return nullptr;
  }

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  return new GLTexture(internal_format, width, height,
                       format, type, data);
}

static GLTexture *
LoadPNG(png_structp png_ptr, png_infop info_ptr,
        const void *data, size_t size)
{
  PNGCallbackContext ctx{(const uint8_t *)data};

  png_set_read_fn(png_ptr, &ctx, PNGReadCallback);

  png_read_info(png_ptr, info_ptr);

  png_uint_32 width, height;
  int bit_depth, color_type, interlace_type;
  png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth,
               &color_type, &interlace_type, nullptr, nullptr);

  /* shrink 16 bit to 8 bit */
  png_set_strip_16(png_ptr);

  /* grow 1,2,4 bit to 8 bit */
  png_set_expand(png_ptr);

  if (color_type == PNG_COLOR_TYPE_PALETTE)
    /* no thanks, we don't want a palette, give us RGB or gray
       instead */
    png_set_palette_to_rgb(png_ptr);

  png_read_update_info(png_ptr, info_ptr);
  png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth,
               &color_type, &interlace_type, NULL, NULL);

  /* allocate memory for the uncompressed pixels */

  const unsigned num_channels = png_get_channels(png_ptr, info_ptr);
  const unsigned pitch = (num_channels * bit_depth) / 8 * width;

  uint8_t *uncompressed = new uint8_t[pitch * height];
  if (uncompressed == nullptr)
    return nullptr;

  png_bytep *rows = new png_bytep[height];
  if (rows == nullptr) {
    delete[] uncompressed;
    return nullptr;
  }

  for (unsigned i = 0; i < height; ++i)
    rows[i] = uncompressed + i * pitch;

  /* uncompress and import into an OpenGL texture */

  png_read_image(png_ptr, rows);
  delete[] rows;

  GLTexture *texture = ImportPNG(width, height, color_type, uncompressed);
  delete[] uncompressed;

  return texture;
}

bool
Bitmap::Load(unsigned id, Type type)
{
  assert(IsScreenInitialized());

  Reset();

  ResourceLoader::Data data = ResourceLoader::Load(id);
  if (data.first == nullptr)
    return false;

  png_structp png_ptr =
    png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  if (png_ptr == nullptr)
    return false;

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (info_ptr == nullptr) {
    png_destroy_read_struct(&png_ptr, nullptr, nullptr);
    return false;
  }

  texture = LoadPNG(png_ptr, info_ptr, data.first, data.second);
  png_destroy_read_struct(&png_ptr, nullptr, nullptr);
  if (texture == nullptr)
    return false;

  size = texture->GetSize();
  return true;
}

#ifdef USE_EGL
bool
Bitmap::LoadStretch(unsigned id, unsigned zoom)
{
  assert(zoom > 0);

  // XXX
  return Load(id);
}
#endif

#ifndef USE_LIBJPEG

bool
Bitmap::LoadFile(const TCHAR *path)
{
  // TODO: use libjpeg when SDL_image is not available
  return false;
}

#endif
