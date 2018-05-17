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

#ifndef XCSOAR_SCREEN_MEMORY_EXPORT_HPP
#define XCSOAR_SCREEN_MEMORY_EXPORT_HPP

#include "PixelTraits.hpp"
#include "Screen/PortableColor.hpp"

#ifdef DITHER
class Dither;
#endif

template<typename PixelTraits>
struct ConstImageBuffer;

static inline uint32_t
GreyscaleToRGB8(Luminosity8 luminosity)
{
  const unsigned value = luminosity.GetLuminosity();

  return value | (value << 8) | (value << 16) | (value << 24);
}

static inline void
CopyGreyscaleToRGB8(uint32_t *gcc_restrict dest,
                     const Luminosity8 *gcc_restrict src,
                     unsigned width)
{
  for (unsigned i = 0; i < width; ++i)
    *dest++ = GreyscaleToRGB8(*src++);
}

static inline RGB565Color
GreyscaleToRGB565(Luminosity8 luminosity)
{
  const unsigned value = luminosity.GetLuminosity();

  return RGB565Color(value, value, value);
}

static inline void
CopyGreyscaleToRGB565(RGB565Color *gcc_restrict dest,
                      const Luminosity8 *gcc_restrict src,
                      unsigned width)
{
  for (unsigned i = 0; i < width; ++i)
    *dest++ = GreyscaleToRGB565(*src++);
}

static constexpr inline RGB565Color
ToRGB565(BGRA8Color c)
{
  return RGB565Color(c.Red(), c.Green(), c.Blue());
}

static inline void
BGRAToRGB565(RGB565Color *dest, const BGRA8Color *src, unsigned n)
{
  for (unsigned i = 0; i < n; ++i)
    dest[i] = ToRGB565(src[i]);
}

#ifdef GREYSCALE

void
CopyFromGreyscale(
#ifdef DITHER
                  Dither &dither,
#endif
#ifdef KOBO
                  bool enable_dither,
#endif
                  void *dest_pixels, unsigned dest_pitch, unsigned dest_bpp,
                  ConstImageBuffer<GreyscalePixelTraits> src);

#else

void
CopyFromBGRA(void *_dest_pixels, unsigned _dest_pitch, unsigned dest_bpp,
             ConstImageBuffer<BGRAPixelTraits> src);

#endif

#endif
