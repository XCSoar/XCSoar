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

#include "RotateDisplay.hpp"

#ifdef ANDROID
#include "Android/Main.hpp"
#include "Android/NativeView.hpp"
#include "Android/Product.hpp"
#endif

#ifdef KOBO
#include "OS/FileUtil.hpp"
#endif

#ifdef WIN32
#include "Screen/RootDC.hpp"

#include <windows.h>
#endif

#if defined(DM_DISPLAYORIENTATION) && defined(_WIN32_WCE) && _WIN32_WCE >= 0x400
/* on PPC2000, ChangeDisplaySettingsEx() crashes silently */
#define ROTATE_SUPPORTED
#endif

#ifdef ROTATE_SUPPORTED
namespace Display {
  bool native_landscape = false;
  DWORD initial_orientation;
}
#endif

void
Display::RotateInitialize()
{
#ifdef ROTATE_SUPPORTED
  DEVMODE DeviceMode;
  memset(&DeviceMode, 0, sizeof(DeviceMode));
  DeviceMode.dmSize = sizeof(DeviceMode);
  DeviceMode.dmFields = DM_DISPLAYORIENTATION;

  // get current rotation
  if (ChangeDisplaySettingsEx(NULL, &DeviceMode, NULL, CDS_TEST, NULL) ==
      DISP_CHANGE_SUCCESSFUL)
    initial_orientation = DeviceMode.dmDisplayOrientation;
  else
    initial_orientation = DMDO_0;

  // determine current screen dimensions
  bool landscape = GetSystemMetrics(SM_CXSCREEN) > GetSystemMetrics(SM_CYSCREEN);

  switch (initial_orientation) {
  case DMDO_90:
  case DMDO_270:
    native_landscape = !landscape;
    break;

  case DMDO_0:
  case DMDO_180:
  default:
    native_landscape = landscape;
    break;
  }
#endif
}

bool
Display::RotateSupported()
{
#ifdef ROTATE_SUPPORTED
  if (GetSystemMetrics(SM_CXSCREEN) == GetSystemMetrics(SM_CYSCREEN))
    /* cannot rotate a square display */
    return false;

  DEVMODE dm;
  memset(&dm, 0, sizeof(dm));
  dm.dmSize = sizeof(dm);
  dm.dmFields = DM_DISPLAYQUERYORIENTATION;

  if (ChangeDisplaySettingsEx(NULL, &dm, NULL, CDS_TEST, NULL) !=
      DISP_CHANGE_SUCCESSFUL)
    return false;

  return dm.dmDisplayOrientation != DMDO_0;
#elif defined(ANDROID) || defined(KOBO)
  return true;
#else
  return false;
#endif
}

bool
Display::Rotate(DisplaySettings::Orientation orientation)
{
#if !defined(ANDROID) && !defined(KOBO)
  if (orientation == DisplaySettings::Orientation::DEFAULT)
    /* leave it as it is */
    return true;
#endif

#ifdef ROTATE_SUPPORTED
  DEVMODE DeviceMode;
  memset(&DeviceMode, 0, sizeof(DeviceMode));
  DeviceMode.dmSize = sizeof(DeviceMode);
  DeviceMode.dmFields = DM_DISPLAYORIENTATION;

  /* determine the new rotation */

  switch (orientation) {
  case DisplaySettings::Orientation::PORTRAIT:
    DeviceMode.dmDisplayOrientation = native_landscape
      ? DMDO_90
      : initial_orientation;
    break;

  case DisplaySettings::Orientation::LANDSCAPE:
    DeviceMode.dmDisplayOrientation = native_landscape
      ? initial_orientation
      : DMDO_270;
    break;

  case DisplaySettings::Orientation::REVERSE_PORTRAIT:
    DeviceMode.dmDisplayOrientation = (native_landscape ? DMDO_270 : DMDO_180);
    break;

  case DisplaySettings::Orientation::REVERSE_LANDSCAPE:
    DeviceMode.dmDisplayOrientation = (native_landscape ? DMDO_180 : DMDO_90);
    break;

  default:
    return false;
  }

  /* apply the new rotation */

  return ChangeDisplaySettingsEx(NULL, &DeviceMode, NULL,
                                 CDS_RESET, NULL) == DISP_CHANGE_SUCCESSFUL;
#elif defined(ANDROID)
  if (native_view == NULL)
    return false;

  NativeView::ScreenOrientation android_orientation;
  switch (orientation) {
  case DisplaySettings::Orientation::PORTRAIT:
    android_orientation = NativeView::ScreenOrientation::PORTRAIT;
    break;

  case DisplaySettings::Orientation::LANDSCAPE:
    android_orientation = NativeView::ScreenOrientation::LANDSCAPE;
    break;

  case DisplaySettings::Orientation::REVERSE_PORTRAIT:
    android_orientation = IsGalaxyTab22() ?
                          NativeView::ScreenOrientation::REVERSE_PORTRAIT_GT :
                          NativeView::ScreenOrientation::REVERSE_PORTRAIT;
    break;

  case DisplaySettings::Orientation::REVERSE_LANDSCAPE:
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
  case DisplaySettings::Orientation::DEFAULT:
  case DisplaySettings::Orientation::PORTRAIT:
    break;

  case DisplaySettings::Orientation::REVERSE_PORTRAIT:
    rotate = "1";
    break;

  case DisplaySettings::Orientation::LANDSCAPE:
    rotate = "0";
    break;

  case DisplaySettings::Orientation::REVERSE_LANDSCAPE:
    rotate = "2";
    break;
  };

  return File::WriteExisting("/sys/class/graphics/fb0/rotate", rotate);
#else
  return false;
#endif
}

bool
Display::RotateRestore()
{
#ifdef ROTATE_SUPPORTED
  DEVMODE dm;
  memset(&dm, 0, sizeof(dm));
  dm.dmSize = sizeof(dm);
  dm.dmFields = DM_DISPLAYORIENTATION;
  dm.dmDisplayOrientation = initial_orientation;

  return ChangeDisplaySettingsEx(NULL, &dm, NULL,
                                 CDS_RESET, NULL) == DISP_CHANGE_SUCCESSFUL;
#elif defined(ANDROID)
  return native_view->setRequestedOrientation(NativeView::ScreenOrientation::SENSOR);
#elif defined(KOBO)
  return Rotate(DisplaySettings::Orientation::DEFAULT);
#else
  return false;
#endif
}
