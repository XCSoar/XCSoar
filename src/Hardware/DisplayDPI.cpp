/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#ifdef ANDROID
#include "Android/Main.hpp"
#include "Android/NativeView.hpp"
#endif

#ifdef KOBO
#include "OS/FileUtil.hpp"
#endif

#ifdef WIN32
#include "Screen/GDI/RootDC.hpp"
#include "Config/Registry.hpp"
#include "OS/GlobalEvent.hpp"

#include <windows.h>
#endif

#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#else
#import <AppKit/AppKit.h>
#endif
#endif

#if !defined(ANDROID) && !defined(_WIN32_WCE)
  static unsigned forced_x_dpi = 0;
  static unsigned forced_y_dpi = 0;
#endif

#if !defined(WIN32) && !defined(ANDROID)
#ifndef __APPLE__
gcc_const
#endif
static unsigned
GetDPI()
{
#ifdef KOBO
  /* Kobo Mini 200 dpi; Kobo Glo 212 dpi (according to Wikipedia) */
  return 200;
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
Display::SetDPI(unsigned x_dpi, unsigned y_dpi)
{
#if !defined(ANDROID) && !defined(_WIN32_WCE)
  forced_x_dpi = x_dpi;
  forced_y_dpi = y_dpi;
#endif
}

unsigned
Display::GetXDPI()
{
#if !defined(ANDROID) && !defined(_WIN32_WCE)
  if (forced_x_dpi > 0)
    return forced_x_dpi;
#endif
#ifdef WIN32
  RootDC dc;
  return GetDeviceCaps(dc, LOGPIXELSX);
#elif defined(ANDROID)
  return native_view->GetXDPI();
#else
  return GetDPI();
#endif
}

unsigned
Display::GetYDPI()
{
#if !defined(ANDROID) && !defined(_WIN32_WCE)
  if (forced_y_dpi > 0)
    return forced_y_dpi;
#endif
#ifdef WIN32
  RootDC dc;
  return GetDeviceCaps(dc, LOGPIXELSY);
#elif defined(ANDROID)
  return native_view->GetYDPI();
#else
  return GetDPI();
#endif
}
