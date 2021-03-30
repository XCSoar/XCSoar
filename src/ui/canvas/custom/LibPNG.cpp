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

#include "LibPNG.hpp"
#include "UncompressedImage.hpp"
#include "system/Path.hpp"
#include "system/FileMapping.hpp"

#include <png.h>

#include <cassert>
#include <stdexcept>

#include <string.h>

struct PNGCallbackContext {
  const uint8_t *data;
};

static void
PNGReadCallback(png_structp _ctx, png_bytep area, png_size_t size) noexcept
{
  PNGCallbackContext &ctx = *(PNGCallbackContext *)png_get_io_ptr(_ctx);

  memcpy(area, ctx.data, size);
  ctx.data += size;
}

[[noreturn]]
static void
PNGUserErrorCallback(png_structp, png_const_charp error_msg)
{
  throw std::runtime_error(error_msg);
}

static void
PNGUserWarningCallback(png_structp, png_const_charp) noexcept
{
}

static constexpr UncompressedImage::Format
ConvertColorType(int color_type) noexcept
{
  switch (color_type) {
  case PNG_COLOR_TYPE_GRAY:
    return UncompressedImage::Format::GRAY;

  case PNG_COLOR_TYPE_RGB:
    return UncompressedImage::Format::RGB;

  case PNG_COLOR_TYPE_RGB_ALPHA:
    return UncompressedImage::Format::RGBA;

  default:
    /* not supported */
    return UncompressedImage::Format::INVALID;
  }
}

static UncompressedImage
LoadPNG(png_structp png_ptr, png_infop info_ptr,
        const void *data, size_t size)
{
  assert(data != nullptr);

  PNGCallbackContext ctx{(const uint8_t *)data};

  png_set_read_fn(png_ptr, &ctx, PNGReadCallback);
  png_set_error_fn(png_ptr, png_get_error_ptr(png_ptr),
                   PNGUserErrorCallback, PNGUserWarningCallback);

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
               &color_type, &interlace_type, nullptr, nullptr);

  /* check the color type */

  const UncompressedImage::Format format = ConvertColorType(color_type);
  if (format == UncompressedImage::Format::INVALID)
    throw std::runtime_error("Unsupported PNG color type");

  /* allocate memory for the uncompressed pixels */

  const unsigned num_channels = png_get_channels(png_ptr, info_ptr);
  const unsigned pitch = (num_channels * bit_depth) / 8 * width;

  std::unique_ptr<uint8_t[]> uncompressed(new uint8_t[pitch * height]);
  png_bytep *rows = new png_bytep[height];

  for (unsigned i = 0; i < height; ++i)
    rows[i] = uncompressed.get() + i * pitch;

  /* uncompress and import into an OpenGL texture */

  png_read_image(png_ptr, rows);
  delete[] rows;

  return UncompressedImage(format, pitch, width, height,
                           std::move(uncompressed));
}

UncompressedImage
LoadPNG(const void *data, size_t size)
{
  png_structp png_ptr =
    png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  if (png_ptr == nullptr)
    throw std::runtime_error("png_create_read_struct() failed");

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (info_ptr == nullptr) {
    png_destroy_read_struct(&png_ptr, nullptr, nullptr);
    throw std::runtime_error("png_create_info_struct() failed");
  }

  UncompressedImage result = LoadPNG(png_ptr, info_ptr, data, size);
  png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);

  return result;
}

UncompressedImage
LoadPNG(Path path)
{
  FileMapping map(path);
  return LoadPNG(map.data(), map.size());
}
