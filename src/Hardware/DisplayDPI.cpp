// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DisplayDPI.hpp"
#include "ui/dim/Size.hpp"
#include "ui/display/Display.hpp"
#include "Math/Point2D.hpp"

#ifdef KOBO
#include "Kobo/Model.hpp"
#endif

#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#else
#import <AppKit/AppKit.h>
#endif
#endif

#include <cassert>
#include <cmath>
#include <cstdint>

UnsignedPoint2D
Display::AlignDpiToPixelAxes(PixelSize size, UnsignedPoint2D dpi) noexcept
{
  if (size.width == 0 || size.height == 0 ||
      size.width == size.height ||
      dpi.x == 0 || dpi.y == 0)
    return dpi;

  const bool pixel_portrait = size.height > size.width;
  const bool inch_portrait =
    (uint64_t)size.height * dpi.x > (uint64_t)size.width * dpi.y;
  if (pixel_portrait == inch_portrait)
    return dpi;

  /* Physical width/height came from the unrotated panel.  Pair the
     shorter physical edge with the shorter pixel edge. */
  const unsigned x = (unsigned)((uint64_t)size.width * dpi.y / size.height);
  const unsigned y = (unsigned)((uint64_t)size.height * dpi.x / size.width);
  if (x == 0 || y == 0)
    return dpi;

  return {x, y};
}

static constexpr bool
DpiFarFrom(unsigned dpi, unsigned density) noexcept
{
  if (density == 0)
    return false;

  const unsigned slack = density / 5;
  return dpi > density + slack || density > dpi + slack;
}

UnsignedPoint2D
Display::SanitizeDisplayDpi(PixelSize size, UnsignedPoint2D physical,
                            unsigned density_dpi) noexcept
{
  const auto dpi = AlignDpiToPixelAxes(size, physical);
  if (DpiFarFrom(dpi.x, density_dpi) || DpiFarFrom(dpi.y, density_dpi))
    return {density_dpi, density_dpi};

  return dpi;
}

float
Display::AndroidTextScaleX(PixelSize size, UnsignedPoint2D physical,
                           unsigned density_dpi,
                           UnsignedPoint2D corrected) noexcept
{
  const auto aligned = AlignDpiToPixelAxes(size, physical);
  if (!DpiFarFrom(aligned.x, density_dpi) &&
      !DpiFarFrom(aligned.y, density_dpi))
    return 1.f;

  if (aligned.x == 0 || aligned.y == 0 ||
      corrected.x == 0 || corrected.y == 0)
    return 1.f;

  const double sx = double(corrected.x) / aligned.x;
  const double sy = double(corrected.y) / aligned.y;
  return float(std::sqrt(sx * sy));
}

float
Display::AndroidTextScaleY(PixelSize size, UnsignedPoint2D physical,
                           unsigned density_dpi,
                           UnsignedPoint2D corrected) noexcept
{
  const auto aligned = AlignDpiToPixelAxes(size, physical);
  if (!DpiFarFrom(aligned.x, density_dpi) &&
      !DpiFarFrom(aligned.y, density_dpi))
    return 1.f;

  if (aligned.y == 0 || corrected.y == 0)
    return 1.f;

  return float(double(corrected.y) / aligned.y);
}

float
Display::AndroidTextLetterSpacing(PixelSize size, UnsignedPoint2D physical,
                                  unsigned density_dpi,
                                  UnsignedPoint2D corrected) noexcept
{
  const float scale_x = AndroidTextScaleX(size, physical,
                                          density_dpi, corrected);
  if (scale_x <= 0.f || scale_x >= 1.f)
    return 0.f;

  return 1.f / scale_x - 1.f;
}

#ifndef ANDROID
static UnsignedPoint2D forced_dpi{};
#endif

#ifdef HAVE_DPI_DETECTION
static UnsignedPoint2D detected_dpi{};
#endif

#if defined(USE_X11) || defined(USE_WAYLAND) || defined(MESA_KMS) || defined(HAVE_DPI_DETECTION)

static constexpr unsigned
MMToDPI(unsigned pixels, unsigned mm)
{
  /* 1 inch = 25.4 mm */
  return pixels * 254 / (mm * 10);
}

[[gnu::const]]
static UnsignedPoint2D
SizeMMToDPI(PixelSize size, PixelSize mm) noexcept
{
  if (size.width == 0 || size.height == 0 ||
      mm.width < 10 || mm.height < 10)
    return {96, 96};

  return {
    MMToDPI(size.width, mm.width),
    MMToDPI(size.height, mm.height),
  };
}

#endif

#if !defined(_WIN32) && !defined(USE_X11) && !defined(USE_WAYLAND) && !defined(MESA_KMS)
#ifndef __APPLE__
[[gnu::const]]
#endif
static unsigned
GetDPI()
{
#ifdef KOBO
  switch (DetectKoboModel()) {
  case KoboModel::GLO_HD:
  case KoboModel::CLARA_HD:
  case KoboModel::CLARA_2E:
  case KoboModel::LIBRA2:
  case KoboModel::LIBRA_H2O:
    return 300;

  case KoboModel::TOUCH2:
    return 167;

  default:
    /* Kobo Mini 200 dpi; Kobo Glo 212 dpi (according to Wikipedia) */
    return 200;
  }
#elif defined(__APPLE__)
#if TARGET_OS_IPHONE
  UIScreen *screen = [UIScreen mainScreen];
  float scale = [screen scale];
  return static_cast<unsigned>(scale * 160);
#else
  NSScreen *screen = [NSScreen mainScreen];
  float scale = [screen backingScaleFactor];
  return static_cast<unsigned>(scale * 115);
#endif
#else
  return 96;
#endif
}
#endif

void
Display::SetForcedDPI([[maybe_unused]] unsigned x_dpi, [[maybe_unused]] unsigned y_dpi)
{
#ifndef ANDROID
  forced_dpi = {x_dpi, y_dpi};
#endif
}

#ifdef HAVE_DPI_DETECTION

void
Display::ProvideDPI(unsigned x_dpi, unsigned y_dpi) noexcept
{
  detected_dpi = {x_dpi, y_dpi};
}

void
Display::ProvideSizeMM(unsigned width_pixels, unsigned height_pixels,
                       unsigned width_mm, unsigned height_mm) noexcept
{
  assert(width_pixels > 0);
  assert(height_pixels > 0);
  assert(width_mm > 0);
  assert(height_mm > 0);

  detected_dpi = AlignDpiToPixelAxes({width_pixels, height_pixels},
                                     SizeMMToDPI({width_pixels, height_pixels},
                                                 {width_mm, height_mm}));
}

#endif

PixelSize
Display::GetSizeForDPI([[maybe_unused]] const UI::Display &display) noexcept
{
#ifdef USE_WAYLAND
  const auto hardware = display.GetHardwareSize();
  if (hardware.width > 0 && hardware.height > 0)
    return hardware;
#endif
#if defined(USE_X11) || defined(USE_WAYLAND) || defined(MESA_KMS)
  return display.GetSize();
#else
  return {};
#endif
}

UnsignedPoint2D
Display::GetDPI([[maybe_unused]] const UI::Display &display, unsigned custom_dpi) noexcept
{
#ifndef ANDROID
  if (forced_dpi.x > 0 && forced_dpi.y > 0)
    return forced_dpi;
#endif

  if (custom_dpi)
    return {custom_dpi, custom_dpi};

#ifdef HAVE_DPI_DETECTION
  if (detected_dpi.x > 0 && detected_dpi.y > 0)
    return detected_dpi;
#endif


#ifdef _WIN32
  return display.GetDPI();
#elif defined(USE_X11) || defined(USE_WAYLAND) || defined(MESA_KMS)
  return SizeMMToDPI(GetSizeForDPI(display), display.GetSizeMM());
#else
  const auto dpi = ::GetDPI();
  return {dpi, dpi};
#endif
}
