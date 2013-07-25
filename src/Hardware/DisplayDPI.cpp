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

#include "DisplayDPI.hpp"

#ifdef ANDROID
#include "Android/Main.hpp"
#include "Android/NativeView.hpp"
#endif

#ifdef KOBO
#include "OS/FileUtil.hpp"
#endif

#ifdef WIN32
#include "Screen/RootDC.hpp"
#include "Config/Registry.hpp"
#include "OS/GlobalEvent.hpp"

#include <windows.h>
#endif

#if !defined(ANDROID) && !defined(_WIN32_WCE)
  static unsigned forced_x_dpi = 0;
  static unsigned forced_y_dpi = 0;
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
#elif defined(KOBO)
  /* Kobo Mini 200 dpi; Kobo Glo 212 dpi (according to Wikipedia) */
  return 200;
#else
  return 96;
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
#elif defined(KOBO)
  /* Kobo Mini 200 dpi; Kobo Glo 212 dpi (according to Wikipedia) */
  return 200;
#else
  return 96;
#endif
}
