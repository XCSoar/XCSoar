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

#include "Screen/Custom/TopCanvas.hpp"
#include "Screen/Canvas.hpp"

#ifdef USE_FB
#include "Screen/Memory/Export.hpp"
#endif

#if defined(KOBO) && defined(USE_FB)
#include "Kobo/Model.hpp"
#include "mxcfb.h"
#endif

#include <algorithm>

#ifdef USE_FB
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

static unsigned
TranslateDimension(unsigned value)
{
#ifdef KOBO
  if (value == 1024 && DetectKoboModel() == KoboModel::AURA)
    /* the Kobo Aura announces 1024 pixel rows, but the physical
       display only shows 1014 */
    value -= 10;
#endif

  return value;
}

static unsigned
GetWidth(const struct fb_var_screeninfo &vinfo)
{
  return TranslateDimension(vinfo.xres);
}

static unsigned
GetHeight(const struct fb_var_screeninfo &vinfo)
{
  return TranslateDimension(vinfo.yres);
}

static PixelSize
GetSize(const struct fb_var_screeninfo &vinfo)
{
  return PixelSize(GetWidth(vinfo), GetHeight(vinfo));
}

#endif

void
TopCanvas::Destroy()
{
  buffer.Free();

#ifdef USE_FB
#ifdef USE_TTY
  DeinitialiseTTY();
#endif

  if (fd >= 0) {
    close(fd);
    fd = -1;
  }
#endif
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
#ifdef USE_FB
  assert(fd < 0);

#ifdef USE_TTY
  InitialiseTTY();
#endif

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

#ifdef GREYSCALE
  /* switch the frame buffer to 8 bits per pixel greyscale */

  vinfo.bits_per_pixel = 8;
  vinfo.grayscale = true;

  if (ioctl(fd, FBIOPUT_VSCREENINFO, &vinfo) < 0) {
    fprintf(stderr, "Couldn't set greyscale pixel format: %s\n",
            strerror(errno));
    Destroy();
    return;
  }

  /* read new finfo */
  ioctl(fd, FBIOGET_FSCREENINFO, &finfo);

  map_bpp = 1;
#else
  map_bpp = vinfo.bits_per_pixel / 8;
  if (map_bpp != 2 && map_bpp != 4) {
    fprintf(stderr, "Unsupported console hardware\n");
    Destroy();
    return;
  }
#endif

  map_pitch = finfo.line_length;
  epd_update_marker = 0;

#ifdef KOBO
  ioctl(fd, MXCFB_SET_UPDATE_SCHEME, UPDATE_SCHEME_QUEUE_AND_MERGE);

  switch(DetectKoboModel()) {
  case KoboModel::UNKNOWN:
  case KoboModel::MINI:
  case KoboModel::TOUCH:
  case KoboModel::GLO:
  case KoboModel::AURA:
    frame_sync = false;
    break;

  case KoboModel::TOUCH2:
  case KoboModel::GLO_HD:
  case KoboModel::AURA2:
    frame_sync = true;
    break;

  };
#endif

  new_size = ::GetSize(vinfo);
#elif defined(USE_VFB)
  /* allocate buffer as requested by caller */
#else
#error No implementation
#endif

  buffer.Allocate(new_size.cx, new_size.cy);
}

#ifdef USE_FB

inline PixelSize
TopCanvas::GetNativeSize() const
{
  struct fb_var_screeninfo vinfo;
  ioctl(fd, FBIOGET_VSCREENINFO, &vinfo);
  return ::GetSize(vinfo);
}

bool
TopCanvas::CheckResize()
{
  return CheckResize(GetNativeSize());
}

#endif

bool
TopCanvas::CheckResize(const PixelSize new_native_size)
{
  const PixelSize new_size = new_native_size;
  if (new_size == GetSize())
    /* no change */
    return false;

  /* changed: update the size and allocate a new buffer */

#ifdef USE_FB
  struct fb_fix_screeninfo finfo;
  ioctl(fd, FBIOGET_FSCREENINFO, &finfo);

  map_pitch = finfo.line_length;
#endif

  buffer.Free();
  buffer.Allocate(new_size.cx, new_size.cy);
  return true;
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

void
TopCanvas::Flip()
{
#ifdef USE_FB

#ifdef GREYSCALE
  CopyFromGreyscale(
#ifdef DITHER
                    dither,
#endif
#ifdef KOBO
                    enable_dither,
#endif
                    map, map_pitch, map_bpp,
                    buffer);
#else
  CopyFromBGRA(map, map_pitch, map_bpp, buffer);
#endif


#ifdef KOBO
  if (frame_sync)
    Wait();

  epd_update_marker++;

  struct mxcfb_update_data epd_update_data = {
    {
      0, 0, buffer.width, buffer.height
    },

    uint32_t(enable_dither &&
             (/* use A2 mode only on some Kobo models */
              DetectKoboModel() == KoboModel::TOUCH2 ||
              DetectKoboModel() == KoboModel::GLO_HD ||
              DetectKoboModel() == KoboModel::AURA2)
             ? WAVEFORM_MODE_A2
             : WAVEFORM_MODE_AUTO),
    UPDATE_MODE_FULL, // PARTIAL
    epd_update_marker,
    TEMP_USE_AMBIENT,
    enable_dither ? EPDC_FLAG_FORCE_MONOCHROME : 0,
  };

  ioctl(fd, MXCFB_SEND_UPDATE, &epd_update_data);
#endif

#endif /* USE_FB */
}

#ifdef KOBO

void
TopCanvas::Wait()
{
  ioctl(fd, MXCFB_WAIT_FOR_UPDATE_COMPLETE, &epd_update_marker);
}

#endif
