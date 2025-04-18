// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Concepts.hpp"
#include "PixelTraits.hpp"
#include "ui/canvas/PortableColor.hpp"
#include "util/Compiler.h"

#ifdef DITHER
class Dither;
#endif

template<AnyPixelTraits PixelTraits>
struct ConstImageBuffer;

constexpr uint32_t
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

constexpr RGB565Color
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
