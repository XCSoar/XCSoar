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

#include "RotateDisplay.hpp"
#include "DisplayOrientation.hpp"

#ifdef ANDROID
#include "Android/Main.hpp"
#include "Android/NativeView.hpp"
#include "Android/Product.hpp"
#endif

#ifdef KOBO
#include "OS/FileUtil.hpp"
#endif

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Features.hpp"
#ifdef SOFTWARE_ROTATE_DISPLAY
#include "UIGlobals.hpp"
#include "Screen/SingleWindow.hpp"
#include "Screen/OpenGL/Globals.hpp"
#endif
#endif

#ifdef WIN32
#include <windows.h>
#include <string.h>
#endif

void
Display::RotateInitialize()
{
}

bool
Display::RotateSupported()
{
#if defined(ANDROID) || defined(KOBO)
  return true;
#elif defined(SOFTWARE_ROTATE_DISPLAY)
  /* rotate supported via glRotatef() (OpenGL projection matrix) */

  /* we need FBO so BufferCanvas can avoid using Canvas::CopyToTexture() */
  return true;
#else
  return false;
#endif
}

bool
Display::Rotate(DisplayOrientation orientation)
{
#if !defined(ANDROID) && !defined(KOBO)
  if (orientation == DisplayOrientation::DEFAULT)
    /* leave it as it is */
    return true;
#endif

#if defined(ANDROID)
  if (native_view == nullptr)
    return false;

  NativeView::ScreenOrientation android_orientation;
  switch (orientation) {
  case DisplayOrientation::PORTRAIT:
    android_orientation = NativeView::ScreenOrientation::PORTRAIT;
    break;

  case DisplayOrientation::LANDSCAPE:
    android_orientation = NativeView::ScreenOrientation::LANDSCAPE;
    break;

  case DisplayOrientation::REVERSE_PORTRAIT:
    android_orientation = IsGalaxyTab22() ?
                          NativeView::ScreenOrientation::REVERSE_PORTRAIT_GT :
                          NativeView::ScreenOrientation::REVERSE_PORTRAIT;
    break;

  case DisplayOrientation::REVERSE_LANDSCAPE:
    android_orientation = IsGalaxyTab22() ?
                          NativeView::ScreenOrientation::REVERSE_LANDSCAPE_GT :
                          NativeView::ScreenOrientation::REVERSE_LANDSCAPE;
    break;

  default:
    android_orientation = NativeView::ScreenOrientation::SENSOR;
  };

  return native_view->setRequestedOrientation(android_orientation);
#elif defined(KOBO)
  const char *rotate = "3";

  switch (orientation) {
  case DisplayOrientation::DEFAULT:
  case DisplayOrientation::PORTRAIT:
    break;

  case DisplayOrientation::REVERSE_PORTRAIT:
    rotate = "1";
    break;

  case DisplayOrientation::LANDSCAPE:
    rotate = "0";
    break;

  case DisplayOrientation::REVERSE_LANDSCAPE:
    rotate = "2";
    break;
  };

  return File::WriteExisting(Path("/sys/class/graphics/fb0/rotate"), rotate);
#elif defined(SOFTWARE_ROTATE_DISPLAY)
  if (!RotateSupported())
    return false;

  UIGlobals::GetMainWindow().SetDisplayOrientation(orientation);
  return true;
#else
  return false;
#endif
}

bool
Display::RotateRestore()
{
#if defined(ANDROID)
  return native_view->setRequestedOrientation(NativeView::ScreenOrientation::SENSOR);
#elif defined(KOBO)
  return Rotate(DisplayOrientation::DEFAULT);
#else
  return false;
#endif
}
