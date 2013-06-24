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

#include "Screen/Custom/TopCanvas.hpp"
#include "Screen/Canvas.hpp"

#ifdef DITHER
#include "Screen/Custom/Dither.hpp"
#endif

#ifdef KOBO
#include "mxcfb.h"
#endif

#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

void
TopCanvas::Destroy()
{
  buffer.Free();

  if (fd >= 0) {
    close(fd);
    fd = -1;
  }
}

PixelRect
TopCanvas::GetRect() const
{
  assert(IsDefined());

  return { 0, 0, int(buffer.width), int(buffer.height) };
}

void
TopCanvas::Create(PixelSize new_size,
                  bool full_screen, bool resizable)
{
  assert(fd < 0);

  const char *path = "/dev/fb0";
  fd = open(path, O_RDWR | O_NOCTTY | O_CLOEXEC);
  if (fd < 0) {
    fprintf(stderr, "Failed to open %s: %s\n", path, strerror(errno));
    return;
  }

  struct fb_fix_screeninfo finfo;
  if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo) < 0) {
    fprintf(stderr, "FBIOGET_FSCREENINFO failed: %s\n", strerror(errno));
    Destroy();
    return;
  }

  if (finfo.type != FB_TYPE_PACKED_PIXELS) {
    fprintf(stderr, "Unsupported console hardware\n");
    Destroy();
    return;
  }

  switch (finfo.visual) {
  case FB_VISUAL_TRUECOLOR:
  case FB_VISUAL_PSEUDOCOLOR:
  case FB_VISUAL_STATIC_PSEUDOCOLOR:
  case FB_VISUAL_DIRECTCOLOR:
    break;

  default:
    fprintf(stderr, "Unsupported console hardware\n");
    Destroy();
    return;
  }

  /* Memory map the device, compensating for buggy PPC mmap() */
  const off_t page_size = getpagesize();
  off_t offset = off_t(finfo.smem_start)
    - (off_t(finfo.smem_start) &~ (page_size - 1));
  off_t map_size = finfo.smem_len + offset;

  map = mmap(nullptr, map_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  if (map == (void *)-1) {
    fprintf(stderr, "Unable to memory map the video hardware: %s\n",
            strerror(errno));
    map = nullptr;
    Destroy();
    return;
  }

  /* Determine the current screen depth */
  struct fb_var_screeninfo vinfo;
  if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) < 0 ) {
    fprintf(stderr, "Couldn't get console pixel format: %s\n",
            strerror(errno));
    Destroy();
    return;
  }

  map_bpp = vinfo.bits_per_pixel / 8;
  if (map_bpp != 2 && map_bpp != 4) {
    fprintf(stderr, "Unsupported console hardware\n");
    Destroy();
    return;
  }

  map_pitch = vinfo.xres_virtual * map_bpp;
  epd_update_marker = 0;

#ifdef KOBO
  ioctl(fd, MXCFB_SET_UPDATE_SCHEME, UPDATE_SCHEME_QUEUE_AND_MERGE);
#endif

  buffer.Allocate(vinfo.xres, vinfo.yres);
}

void
TopCanvas::OnResize(PixelSize new_size)
{
  // TODO: is this even possible?
}

void
TopCanvas::Fullscreen()
{
}

Canvas
TopCanvas::Lock()
{
  return Canvas(buffer);
}

void
TopCanvas::Unlock()
{
}

#ifdef GREYSCALE

#ifdef DITHER

#else

static uint32_t
GreyscaleToRGB8(Luminosity8 luminosity)
{
  const unsigned value = luminosity.GetLuminosity();

  return value | (value << 8) | (value << 16) | (value << 24);
}

static void
CopyGreyscaleToRGB8(uint32_t *gcc_restrict dest,
                     const Luminosity8 *gcc_restrict src,
                     unsigned width)
{
  for (unsigned i = 0; i < width; ++i)
    *dest++ = GreyscaleToRGB8(*src++);
}

static RGB565Color
GreyscaleToRGB565(Luminosity8 luminosity)
{
  const unsigned value = luminosity.GetLuminosity();

  return RGB565Color(value, value, value);
}

static void
CopyGreyscaleToRGB565(RGB565Color *gcc_restrict dest,
                      const Luminosity8 *gcc_restrict src,
                      unsigned width)
{
  for (unsigned i = 0; i < width; ++i)
    *dest++ = GreyscaleToRGB565(*src++);
}

#endif

static void
CopyFromGreyscale(
#ifdef DITHER
                  Dither &dither,
#endif
                  void *dest_pixels, unsigned dest_pitch, unsigned dest_bpp,
                  ConstImageBuffer<GreyscalePixelTraits> src)
{
  const uint8_t *src_pixels = reinterpret_cast<const uint8_t *>(src.data);

  const unsigned width = src.width, height = src.height;

#ifdef DITHER

  dither.dither_luminosity8_to_uint16(src_pixels, src.pitch,
                                      (uint16_t *)dest_pixels,
                                      dest_pitch / dest_bpp,
                                      width, height);
  if (dest_bpp == 4) {
    const unsigned n_pixels = (dest_pitch / dest_bpp)
      * height;
    int32_t *d = (int32_t *)dest_pixels + n_pixels;
    const int16_t *end = (int16_t *)dest_pixels;
    const int16_t *s = end + n_pixels;

    while (s != end)
      *--d = *--s;
  }

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

#endif

void
TopCanvas::Flip()
{
#ifdef GREYSCALE
  CopyFromGreyscale(
#ifdef DITHER
                    dither,
#endif
                    map, map_pitch, map_bpp,
                    buffer);
#else
  // TODO
#endif


#ifdef KOBO
  epd_update_marker++;

  struct mxcfb_update_data epd_update_data = {
    {
      0, 0, buffer.width, buffer.height
    },

    WAVEFORM_MODE_AUTO,
    UPDATE_MODE_FULL, // PARTIAL
    epd_update_marker,
    TEMP_USE_AMBIENT,
    EPDC_FLAG_FORCE_MONOCHROME,
  };

  ioctl(fd, MXCFB_SEND_UPDATE, &epd_update_data);
#endif
}
