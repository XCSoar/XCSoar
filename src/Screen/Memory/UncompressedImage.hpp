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

#ifndef XCSOAR_SCREEN_MEMORY_UNCOMPRESSED_IMAGE_HPP
#define XCSOAR_SCREEN_MEMORY_UNCOMPRESSED_IMAGE_HPP

#include "Buffer.hpp"
#include "Screen/Custom/UncompressedImage.hpp"

struct RGBPixelReader {
  const uint8_t *p;

  template<typename PixelTraits>
  typename PixelTraits::color_type Read(PixelTraits) {
    const uint8_t r = *p++, g = *p++, b = *p++;
    return typename PixelTraits::color_type(r, g, b);
  }
};

struct GrayPixelReader {
  const uint8_t *p;

  template<typename PixelTraits>
  typename PixelTraits::color_type Read(PixelTraits) {
    const uint8_t l = *p++;
    return typename PixelTraits::color_type(l, l, l);
  }
};

template<typename PixelTraits, typename Reader>
static inline void
ConvertLine(typename PixelTraits::rpointer_type dest, Reader src, unsigned n)
{
  for (unsigned i = 0; i < n; ++i, dest = PixelTraits::Next(dest, 1))
    PixelTraits::WritePixel(dest, src.Read(PixelTraits()));
}

template<typename PixelTraits, typename Format>
static inline void
ConvertImage(WritableImageBuffer<PixelTraits> buffer,
             const uint8_t *src, int src_pitch, bool flipped)
{
  typename PixelTraits::rpointer_type dest = buffer.data;

  if (flipped) {
    src += src_pitch * (buffer.height - 1);
    src_pitch = -src_pitch;
  }

  for (unsigned i = 0; i < buffer.height; ++i,
         dest = PixelTraits::NextRow(dest, buffer.pitch, 1),
         src += src_pitch)
    ConvertLine<PixelTraits>(dest, Format{src}, buffer.width);
}

/**
 * Convert an #UncompressedImage to a SDL_Surface.
 *
 * @return the new SDL_Surface object or nullptr on error
 */
template<typename PixelTraits>
static inline void
ImportSurface(WritableImageBuffer<PixelTraits> &buffer,
              const UncompressedImage &uncompressed)
{
  buffer.Allocate(uncompressed.GetWidth(), uncompressed.GetHeight());

  switch (uncompressed.GetFormat()) {
  case UncompressedImage::Format::INVALID:
    assert(false);
    gcc_unreachable();

  case UncompressedImage::Format::RGB:
  case UncompressedImage::Format::RGBA:
    ConvertImage<PixelTraits,
                 RGBPixelReader>(buffer,
                                 (const uint8_t *)uncompressed.GetData(),
                                 uncompressed.GetPitch(),
                                 uncompressed.IsFlipped());
    break;

  case UncompressedImage::Format::GRAY:
    ConvertImage<PixelTraits,
                 GrayPixelReader>(buffer,
                                  (const uint8_t *)uncompressed.GetData(),
                                  uncompressed.GetPitch(),
                                  uncompressed.IsFlipped());
    break;
  }
}

#endif
