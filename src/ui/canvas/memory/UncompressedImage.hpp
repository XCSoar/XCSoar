// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Concepts.hpp"
#include "Buffer.hpp"
#include "ui/canvas/custom/UncompressedImage.hpp"

struct RGBPixelReader {
  const uint8_t *p;

  template<AnyPixelTraits PixelTraits>
  typename PixelTraits::color_type Read(PixelTraits) noexcept {
    const uint8_t r = *p++, g = *p++, b = *p++;
    return typename PixelTraits::color_type(r, g, b);
  }
};

struct GrayPixelReader {
  const uint8_t *p;

  template<AnyPixelTraits PixelTraits>
  typename PixelTraits::color_type Read(PixelTraits) noexcept {
    const uint8_t l = *p++;
    return typename PixelTraits::color_type(l, l, l);
  }
};

template<AnyPixelTraits PixelTraits, typename Reader>
static inline void
ConvertLine(typename PixelTraits::rpointer dest, Reader src,
            unsigned n) noexcept
{
  for (unsigned i = 0; i < n; ++i, dest = PixelTraits::Next(dest, 1))
    PixelTraits::WritePixel(dest, src.Read(PixelTraits()));
}

template<AnyPixelTraits PixelTraits, typename Format>
static inline void
ConvertImage(WritableImageBuffer<PixelTraits> buffer,
             const uint8_t *src, int src_pitch, bool flipped) noexcept
{
  typename PixelTraits::rpointer dest = buffer.data;

  if (flipped) {
    src += src_pitch * (buffer.size.height - 1);
    src_pitch = -src_pitch;
  }

  for (unsigned i = 0; i < buffer.size.height; ++i,
         dest = PixelTraits::NextRow(dest, buffer.pitch, 1),
         src += src_pitch)
    ConvertLine<PixelTraits>(dest, Format{src}, buffer.size.width);
}

/**
 * Convert an #UncompressedImage to a SDL_Surface.
 *
 * @return the new SDL_Surface object or nullptr on error
 */
template<AnyPixelTraits PixelTraits>
static inline void
ImportSurface(WritableImageBuffer<PixelTraits> &buffer,
              const UncompressedImage &uncompressed) noexcept
{
  buffer.Allocate(uncompressed.GetSize());

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
