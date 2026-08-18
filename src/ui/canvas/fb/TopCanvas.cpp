// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/custom/TopCanvas.hpp"
#include "ui/canvas/Canvas.hpp"
#include "lib/fmt/SystemError.hxx"

#ifdef TARGET_IS_KOBO_NICKEL
#include "FBInkBackend.hpp"
#endif

#ifdef USE_FB
#include "ui/canvas/memory/Export.hpp"
#endif

#ifdef USE_FB
#include "Hardware/DisplayDPI.hpp"
#endif

#if defined(KOBO) && defined(USE_FB) && !defined(TARGET_IS_KOBO_NICKEL)
#include "Kobo/Model.hpp"
#include "mxcfb.h"
#endif

#include <algorithm>

#ifdef USE_FB
#ifndef TARGET_IS_KOBO_NICKEL
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#endif
#include <cassert>
#include <string.h>
#ifndef TARGET_IS_KOBO_NICKEL
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#endif

#ifndef TARGET_IS_KOBO_NICKEL
static unsigned
TranslateDimension(unsigned value) noexcept
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
GetWidth(const struct fb_var_screeninfo &vinfo) noexcept
{
  return TranslateDimension(vinfo.xres);
}

static unsigned
GetHeight(const struct fb_var_screeninfo &vinfo) noexcept
{
  return TranslateDimension(vinfo.yres);
}

static PixelSize
GetSize(const struct fb_var_screeninfo &vinfo) noexcept
{
  return PixelSize(GetWidth(vinfo), GetHeight(vinfo));
}
#endif

#endif

TopCanvas::~TopCanvas() noexcept
{
  buffer.Free();

#ifdef USE_FB
#ifndef TARGET_IS_KOBO_NICKEL
  if (fd >= 0) {
    close(fd);
    fd = -1;
  }
#endif
#endif
}

#ifdef USE_FB

TopCanvas::TopCanvas(UI::Display &_display)
  :display(_display)
{
#ifdef TARGET_IS_KOBO_NICKEL
  fbink = std::make_unique<FBInkBackend>();
  const PixelSize new_size = fbink->GetSize();
  if (new_size.width == 0 || new_size.height == 0)
    throw std::runtime_error("FBInk reported an empty display");

  buffer.Allocate(new_size);
#else
  assert(fd < 0);

  const char *path = "/dev/fb0";
  fd = open(path, O_RDWR | O_NOCTTY | O_CLOEXEC);
  if (fd < 0)
    throw FmtErrno("Failed to open {}", path);

  struct fb_fix_screeninfo finfo;
  if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo) < 0)
    throw MakeErrno("FBIOGET_FSCREENINFO failed");

  if (finfo.type != FB_TYPE_PACKED_PIXELS)
    throw std::runtime_error("Unsupported console hardware");

  switch (finfo.visual) {
  case FB_VISUAL_TRUECOLOR:
  case FB_VISUAL_PSEUDOCOLOR:
  case FB_VISUAL_STATIC_PSEUDOCOLOR:
  case FB_VISUAL_DIRECTCOLOR:
    break;

  default:
    throw std::runtime_error("Unsupported console hardware");
  }

  /* Memory map the device, compensating for buggy PPC mmap() */
  const off_t page_size = getpagesize();
  off_t offset = off_t(finfo.smem_start)
    - (off_t(finfo.smem_start) &~ (page_size - 1));
  off_t map_size = finfo.smem_len + offset;

  map = mmap(nullptr, map_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  if (map == (void *)-1)
    throw MakeErrno("Unable to memory map the video hardware");

  /* Determine the current screen depth */
  struct fb_var_screeninfo vinfo;
  if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) < 0)
    throw MakeErrno("Couldn't get console pixel format");

#ifdef GREYSCALE
  /* switch the frame buffer to 8 bits per pixel greyscale */

  vinfo.bits_per_pixel = 8;
  vinfo.grayscale = true;

  if (ioctl(fd, FBIOPUT_VSCREENINFO, &vinfo) < 0)
    throw MakeErrno("Couldn't set greyscale pixel format");

  /* read new finfo */
  ioctl(fd, FBIOGET_FSCREENINFO, &finfo);

  map_bpp = 1;
#else
  map_bpp = vinfo.bits_per_pixel / 8;
  if (map_bpp != 2 && map_bpp != 4)
    throw std::runtime_error("Unsupported console hardware");
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
  case KoboModel::NIA:
  case KoboModel::CLARA_BW:
  case KoboModel::CLARA_COLOUR:
    frame_sync = false;
    break;

  case KoboModel::TOUCH2:
  case KoboModel::GLO_HD:
  case KoboModel::AURA2:
  case KoboModel::CLARA_HD:
  case KoboModel::CLARA_2E:
  case KoboModel::LIBRA2:
  case KoboModel::LIBRA_H2O:
    frame_sync = true;
    break;

  };
#endif

  const auto new_size = ::GetSize(vinfo);

  if (vinfo.width > 0 && vinfo.height > 0)
    Display::ProvideSizeMM(new_size.width, new_size.height,
                           vinfo.width, vinfo.height);

  buffer.Allocate(new_size);
#endif
}

inline PixelSize
TopCanvas::GetNativeSize() const noexcept
{
#ifdef TARGET_IS_KOBO_NICKEL
  return fbink->GetSize();
#else
  struct fb_var_screeninfo vinfo;
  ioctl(fd, FBIOGET_VSCREENINFO, &vinfo);
  return ::GetSize(vinfo);
#endif
}

bool
TopCanvas::CheckResize() noexcept
{
  return CheckResize(GetNativeSize());
}

#elif defined(USE_VFB)

TopCanvas::TopCanvas(UI::Display &_display, PixelSize new_size)
  :display(_display)
{
  buffer.Allocate(new_size);

  // suppress -Wunused
  (void)display;
}

#else
#error No implementation
#endif

bool
TopCanvas::CheckResize(const PixelSize new_native_size) noexcept
{
  const PixelSize new_size = new_native_size;
  if (new_size == GetSize())
    /* no change */
    return false;

  /* changed: update the size and allocate a new buffer */

#ifdef USE_FB
#ifndef TARGET_IS_KOBO_NICKEL
  struct fb_fix_screeninfo finfo;
  ioctl(fd, FBIOGET_FSCREENINFO, &finfo);

  map_pitch = finfo.line_length;
#endif
#endif

  buffer.Free();
  buffer.Allocate(new_size);
  return true;
}

Canvas
TopCanvas::Lock()
{
  return Canvas(buffer);
}

void
TopCanvas::Unlock() noexcept
{
}

void
TopCanvas::Flip()
{
#ifdef USE_FB

#ifdef TARGET_IS_KOBO_NICKEL
  const auto frame_buffer = fbink->Prepare();
  if (frame_buffer.pitch < buffer.size.width * frame_buffer.bytes_per_pixel ||
      frame_buffer.size < std::size_t{frame_buffer.pitch} * buffer.size.height)
    throw std::runtime_error("FBInk framebuffer is smaller than the canvas");

#ifdef GREYSCALE
  CopyFromGreyscale(
#ifdef DITHER
                    dither,
#endif
#ifdef KOBO
                    enable_dither,
#endif
                    frame_buffer.data, frame_buffer.pitch,
                    frame_buffer.bytes_per_pixel, buffer);
#else
  CopyFromBGRA(frame_buffer.data, frame_buffer.pitch,
               frame_buffer.bytes_per_pixel, buffer);
#endif

  fbink->Refresh(buffer.size);
#else

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

  KoboModel kobo_model = DetectKoboModel();
  struct mxcfb_update_data epd_update_data = {
    {
      0, 0, buffer.size.width, buffer.size.height
    },

    uint32_t(enable_dither &&
             (/* use A2 mode only on some Kobo models */
              kobo_model == KoboModel::TOUCH2 ||
              kobo_model == KoboModel::GLO_HD ||
              kobo_model == KoboModel::AURA2 ||
              kobo_model == KoboModel::LIBRA2 ||
              kobo_model == KoboModel::LIBRA_H2O ||
              kobo_model == KoboModel::CLARA_HD ||
              kobo_model == KoboModel::CLARA_2E)
             ? WAVEFORM_MODE_A2
             : WAVEFORM_MODE_AUTO),
    UPDATE_MODE_FULL, // PARTIAL
    epd_update_marker,
    TEMP_USE_AMBIENT,
    enable_dither ? EPDC_FLAG_FORCE_MONOCHROME : 0,
  };

  ioctl(fd, MXCFB_SEND_UPDATE, &epd_update_data);
#endif

#endif /* TARGET_IS_KOBO_NICKEL */
#endif /* USE_FB */
}

#ifdef KOBO

void
TopCanvas::Wait() noexcept
{
#ifndef TARGET_IS_KOBO_NICKEL
  ioctl(fd, MXCFB_WAIT_FOR_UPDATE_COMPLETE, &epd_update_marker);
#endif
}

#endif
