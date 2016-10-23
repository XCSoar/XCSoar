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

#include "Export.hpp"
#include "Buffer.hpp"

#ifdef DITHER
#include "../Memory/Dither.hpp"
#endif

#include <assert.h>

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

#endif /* KOBO */

void
CopyFromGreyscale(
#ifdef DITHER
                  Dither &dither,
#endif
#ifdef KOBO
                  bool enable_dither,
#endif
                  void *dest_pixels, unsigned dest_pitch, unsigned dest_bpp,
                  ConstImageBuffer<GreyscalePixelTraits> src)
{
  const uint8_t *src_pixels = reinterpret_cast<const uint8_t *>(src.data);

  const unsigned width = src.width, height = src.height;

#ifdef KOBO
  if (!enable_dither) {
    CopyGreyscale((uint8_t *)dest_pixels, dest_pitch,
                  src_pixels, src.pitch,
                  width, height);
    return;
  }
#endif

#ifdef DITHER

  dither.DitherGreyscale(src_pixels, src.pitch,
                         (uint8_t *)dest_pixels,
                         dest_pitch,
                         width, height);

#ifndef KOBO
  if (dest_bpp == 4) {
    const unsigned n_pixels = (dest_pitch / dest_bpp)
      * height;
    int32_t *d = (int32_t *)dest_pixels + n_pixels;
    const int8_t *end = (int8_t *)dest_pixels;
    const int8_t *s = end + n_pixels;

    while (s != end)
      *--d = *--s;
  }
#endif

#else

  const unsigned src_pitch = src.pitch;

  if (dest_bpp == 2) {
    for (unsigned row = height; row > 0;
         --row, src_pixels += src_pitch, dest_pixels += dest_pitch)
      CopyGreyscaleToRGB565((RGB565Color *)dest_pixels,
                            (const Luminosity8 *)src_pixels, width);
  } else {
    for (unsigned row = height; row > 0;
         --row, src_pixels += src_pitch, dest_pixels += dest_pitch)
      CopyGreyscaleToRGB8((uint32_t *)dest_pixels,
                           (const Luminosity8 *)src_pixels, width);
  }

#endif
}

#else /* GREYSCALE */

void
CopyFromBGRA(void *_dest_pixels, unsigned _dest_pitch, unsigned dest_bpp,
             ConstImageBuffer<BGRAPixelTraits> src)
{
  assert(dest_bpp == 4 || dest_bpp == 2);

  const uint32_t dest_pitch = _dest_pitch / dest_bpp;
  const uint32_t src_pitch = src.pitch / sizeof(*src.data);

  if (dest_bpp == 2) {
    /* convert to RGB565 */

    RGB565Color *dest_pixels = reinterpret_cast<RGB565Color *>(_dest_pixels);
    const BGRA8Color *src_pixels = src.data;

    for (unsigned row = src.height; row > 0;
         --row, src_pixels += src_pitch, dest_pixels += dest_pitch)
      BGRAToRGB565((RGB565Color *)dest_pixels,
                   (const BGRA8Color *)src_pixels,
                   src.width);
  } else {
    uint32_t *dest_pixels = reinterpret_cast<uint32_t *>(_dest_pixels);
    const uint32_t *src_pixels = reinterpret_cast<const uint32_t *>(src.data);

    for (unsigned row = src.height; row > 0;
         --row, src_pixels += src_pitch, dest_pixels += dest_pitch)
      std::copy_n(src_pixels, src.width, dest_pixels);
  }
}

#endif
