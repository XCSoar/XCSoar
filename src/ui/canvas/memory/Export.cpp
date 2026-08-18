// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Export.hpp"
#include "Buffer.hpp"

#ifdef DITHER
#include "Dither.hpp"
#endif

#include <cassert>

#ifdef GREYSCALE

#ifdef KOBO

static void
CopyGreyscale(uint8_t *dest_pixels, unsigned dest_pitch,
              const uint8_t *src_pixels, unsigned src_pitch,
              unsigned width, unsigned height)
{
  for (unsigned y = 0; y < height;
       ++y, dest_pixels += dest_pitch, src_pixels += src_pitch)
    std::copy_n(src_pixels, width, dest_pixels);
}

#ifdef DITHER
static void
ExpandDitheredGreyscale(void *dest_pixels, unsigned dest_pitch,
                        unsigned dest_bpp, unsigned width, unsigned height)
{
  if (dest_bpp == 2) {
    auto *expanded = static_cast<RGB565Color *>(dest_pixels);
    const unsigned pitch = dest_pitch / sizeof(*expanded);
    for (unsigned y = 0; y < height; ++y, expanded += pitch) {
      const auto *source = reinterpret_cast<const uint8_t *>(expanded);
      for (unsigned x = width; x > 0; --x)
        expanded[x - 1] = GreyscaleToRGB565(Luminosity8{source[x - 1]});
    }
  } else if (dest_bpp == 4) {
    auto *expanded = static_cast<uint32_t *>(dest_pixels);
    const unsigned pitch = dest_pitch / sizeof(*expanded);
    for (unsigned y = 0; y < height; ++y, expanded += pitch) {
      const auto *source = reinterpret_cast<const uint8_t *>(expanded);
      for (unsigned x = width; x > 0; --x)
        expanded[x - 1] = GreyscaleToRGB8(Luminosity8{source[x - 1]});
    }
  }
}
#endif

#endif /* KOBO */

void
CopyFromGreyscale(
#ifdef DITHER
                  Dither &dither,
#endif
#ifdef KOBO
                  bool enable_dither,
#endif
                  void *dest_pixels, unsigned dest_pitch, [[maybe_unused]] unsigned dest_bpp,
                  ConstImageBuffer<GreyscalePixelTraits> src)
{
  const uint8_t *src_pixels = reinterpret_cast<const uint8_t *>(src.data);

#ifdef KOBO
  assert(dest_bpp == 1 || dest_bpp == 2 || dest_bpp == 4);

#ifndef DITHER
  if (dest_bpp == 1) {
    CopyGreyscale((uint8_t *)dest_pixels, dest_pitch,
                  src_pixels, src.pitch,
                  src.size.width, src.size.height);
    return;
  }
#endif

  if (dest_bpp == 1 && !enable_dither) {
    CopyGreyscale((uint8_t *)dest_pixels, dest_pitch,
                  src_pixels, src.pitch,
                  src.size.width, src.size.height);
    return;
  }

#ifdef DITHER
  if (enable_dither) {
    dither.DitherGreyscale(src_pixels, src.pitch,
                           (uint8_t *)dest_pixels, dest_pitch,
                           src.size.width, src.size.height);
    if (dest_bpp != 1)
      ExpandDitheredGreyscale(dest_pixels, dest_pitch, dest_bpp,
                              src.size.width, src.size.height);
    return;
  }
#endif
#endif

#if defined(DITHER) && !defined(KOBO)

  dither.DitherGreyscale(src_pixels, src.pitch,
                         (uint8_t *)dest_pixels,
                         dest_pitch,
                         src.size.width, src.size.height);

  if (dest_bpp == 4) {
    const unsigned n_pixels = (dest_pitch / dest_bpp)
      * src.size.height;
    int32_t *d = (int32_t *)dest_pixels + n_pixels;
    const int8_t *end = (int8_t *)dest_pixels;
    const int8_t *s = end + n_pixels;

    while (s != end)
      *--d = *--s;
  }
  return;
#endif /* DITHER && !KOBO */

  const unsigned src_pitch = src.pitch;

  if (dest_bpp == 2) {
    auto *dest = static_cast<RGB565Color *>(dest_pixels);
    const unsigned dest_pitch_pixels = dest_pitch / sizeof(*dest);
    for (unsigned row = src.size.height; row > 0;
         --row, src_pixels += src_pitch, dest += dest_pitch_pixels)
      CopyGreyscaleToRGB565(dest,
                            (const Luminosity8 *)src_pixels, src.size.width);
  } else {
    auto *dest = static_cast<uint32_t *>(dest_pixels);
    const unsigned dest_pitch_pixels = dest_pitch / sizeof(*dest);
    for (unsigned row = src.size.height; row > 0;
         --row, src_pixels += src_pitch, dest += dest_pitch_pixels)
      CopyGreyscaleToRGB8(dest,
                           (const Luminosity8 *)src_pixels, src.size.width);
  }
}

#else /* GREYSCALE */

void
CopyFromBGRA(void *_dest_pixels, unsigned _dest_pitch, unsigned dest_bpp,
             ConstImageBuffer<BGRAPixelTraits> src) noexcept
{
  assert(dest_bpp == 1 || dest_bpp == 2 || dest_bpp == 4);

  const uint32_t dest_pitch = _dest_pitch / dest_bpp;
  const uint32_t src_pitch = src.pitch / sizeof(*src.data);

  if (dest_bpp == 1) {
    auto *dest_pixels = static_cast<uint8_t *>(_dest_pixels);
    const BGRA8Color *src_pixels = src.data;

    for (unsigned row = src.size.height; row > 0;
         --row, src_pixels += src_pitch, dest_pixels += dest_pitch)
      for (unsigned x = 0; x < src.size.width; ++x)
        dest_pixels[x] = Luminosity8{RGB8Color{src_pixels[x]}}.GetLuminosity();
  } else if (dest_bpp == 2) {
    /* convert to RGB565 */

    RGB565Color *dest_pixels = reinterpret_cast<RGB565Color *>(_dest_pixels);
    const BGRA8Color *src_pixels = src.data;

    for (unsigned row = src.size.height; row > 0;
         --row, src_pixels += src_pitch, dest_pixels += dest_pitch)
      BGRAToRGB565((RGB565Color *)dest_pixels,
                   (const BGRA8Color *)src_pixels,
                   src.size.width);
  } else {
    uint32_t *dest_pixels = reinterpret_cast<uint32_t *>(_dest_pixels);
    const uint32_t *src_pixels = reinterpret_cast<const uint32_t *>(src.data);

    for (unsigned row = src.size.height; row > 0;
         --row, src_pixels += src_pitch, dest_pixels += dest_pitch)
      std::copy_n(src_pixels, src.size.width, dest_pixels);
  }
}

#endif
