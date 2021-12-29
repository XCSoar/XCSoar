/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "DisplayDPI.hpp"

#ifdef _WIN32
#include "ui/canvas/gdi/RootDC.hpp"

#include <wingdi.h>
#endif

#ifdef USE_X11
#include "ui/event/Globals.hpp"
#include "ui/event/Queue.hpp"

#define Font X11Font
#define Window X11Window
#define Display X11Display
#include <X11/Xlib.h>
#undef Font
#undef Window
#undef Display
#endif

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

#ifndef ANDROID
  static unsigned forced_x_dpi = 0;
  static unsigned forced_y_dpi = 0;
#endif

#ifdef HAVE_DPI_DETECTION
static unsigned detected_x_dpi = 0, detected_y_dpi = 0;
#endif

#if defined(USE_X11) || defined(HAVE_DPI_DETECTION)

static constexpr unsigned
MMToDPI(unsigned pixels, unsigned mm)
{
  /* 1 inch = 25.4 mm */
  return pixels * 254 / (mm * 10);
}

#endif

#if !defined(_WIN32) && !defined(USE_X11)
#ifndef __APPLE__
gcc_const
#endif
static unsigned
GetDPI()
{
#ifdef KOBO
  switch (DetectKoboModel()) {
  case KoboModel::GLO_HD:
  case KoboModel::CLARA_HD:
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
Display::SetForcedDPI(unsigned x_dpi, unsigned y_dpi)
{
#ifndef ANDROID
  forced_x_dpi = x_dpi;
  forced_y_dpi = y_dpi;
#endif
}

#ifdef HAVE_DPI_DETECTION

void
Display::ProvideDPI(unsigned x_dpi, unsigned y_dpi) noexcept
{
  detected_x_dpi = x_dpi;
  detected_y_dpi = y_dpi;
}

void
Display::ProvideSizeMM(unsigned width_pixels, unsigned height_pixels,
                       unsigned width_mm, unsigned height_mm) noexcept
{
  assert(width_pixels > 0);
  assert(height_pixels > 0);
  assert(width_mm > 0);
  assert(height_mm > 0);

  detected_x_dpi = MMToDPI(width_pixels, width_mm);
  detected_y_dpi = MMToDPI(height_pixels, height_mm);
}

#endif

unsigned
Display::GetXDPI(unsigned custom_dpi)
{
#ifndef ANDROID
  if (forced_x_dpi > 0)
    return forced_x_dpi;
#endif

  if (custom_dpi)
    return custom_dpi;

#ifdef HAVE_DPI_DETECTION
  if (detected_x_dpi > 0)
    return detected_x_dpi;
#endif


#ifdef _WIN32
  RootDC dc;
  return GetDeviceCaps(dc, LOGPIXELSX);
#elif defined(USE_X11)
  assert(UI::event_queue != nullptr);

  auto display = UI::event_queue->GetDisplay();
  assert(display != nullptr);

  return MMToDPI(DisplayWidth(display, 0), DisplayWidthMM(display, 0));
#else
  return GetDPI();
#endif
}

unsigned
Display::GetYDPI(unsigned custom_dpi)
{
#ifndef ANDROID
  if (forced_y_dpi > 0)
    return forced_y_dpi;
#endif

  if (custom_dpi)
    return custom_dpi;

#ifdef HAVE_DPI_DETECTION
  if (detected_y_dpi > 0)
    return detected_y_dpi;
#endif

#ifdef _WIN32
  RootDC dc;
  return GetDeviceCaps(dc, LOGPIXELSY);
#elif defined(USE_X11)
  assert(UI::event_queue != nullptr);

  auto display = UI::event_queue->GetDisplay();
  assert(display != nullptr);

  return MMToDPI(DisplayHeight(display, 0), DisplayHeightMM(display, 0));
#else
  return GetDPI();
#endif
}
