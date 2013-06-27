/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

template<typename PixelTraits>
static inline void
ConvertFromRGB(typename PixelTraits::rpointer_type dest,
               const uint8_t *src, unsigned n)
{
  for (unsigned i = 0; i < n; ++i, dest = PixelTraits::Next(dest, 1)) {
    const uint8_t r = *src++, g = *src++, b = *src++;
    typename PixelTraits::color_type color(r, g, b);
    PixelTraits::WritePixel(dest, color);
  }
}

template<typename PixelTraits>
static inline void
ConvertFromRGB(WritableImageBuffer<PixelTraits> buffer,
               const uint8_t *src, unsigned src_pitch)
{
  typename PixelTraits::rpointer_type dest = buffer.data;

  for (unsigned i = 0; i < buffer.height; ++i,
         dest = PixelTraits::NextRow(dest, buffer.pitch, 1),
         src += src_pitch)
    ConvertFromRGB<PixelTraits>(dest, src, buffer.width);
}

template<typename PixelTraits>
static inline void
ConvertFromGray(typename PixelTraits::rpointer_type dest,
                const uint8_t *src, unsigned n)
{
  for (unsigned i = 0; i < n; ++i, dest = PixelTraits::Next(dest, 1)) {
    const uint8_t l = *src++;
    typename PixelTraits::color_type color(l, l, l);
    PixelTraits::WritePixel(dest, color);
  }
}

template<typename PixelTraits>
static inline void
ConvertFromGray(WritableImageBuffer<PixelTraits> buffer,
                const uint8_t *src, unsigned src_pitch)
{
  typename PixelTraits::rpointer_type dest = buffer.data;

  for (unsigned i = 0; i < buffer.height; ++i,
         dest = PixelTraits::NextRow(dest, buffer.pitch, 1),
         src += src_pitch)
    ConvertFromGray<PixelTraits>(dest, src, buffer.width);
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
    ConvertFromRGB<PixelTraits>(buffer,
                                (const uint8_t *)uncompressed.GetData(),
                                uncompressed.GetPitch());
    break;

  case UncompressedImage::Format::GRAY:
    ConvertFromGray<PixelTraits>(buffer,
                                 (const uint8_t *)uncompressed.GetData(),
                                 uncompressed.GetPitch());
    break;
  }
}

#endif
