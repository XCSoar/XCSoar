/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Display.hpp"

#include "Asset.hpp"

#ifdef ANDROID
#include "Android/Main.hpp"
#include "Android/NativeView.hpp"
#endif

#ifdef WIN32
#include "Screen/RootDC.hpp"
#include "Config/Registry.hpp"
#include "OS/GlobalEvent.hpp"

#include <windows.h>
#endif

#ifdef HAVE_HARDWARE_BLANK

#include "Hardware/VideoPower.h"

bool
Display::BlankSupported()
{
  RootDC dc;
  int i = SETPOWERMANAGEMENT;
  return ExtEscape(dc, QUERYESCSUPPORT,
                   sizeof(i), (LPCSTR)&i, 0, NULL) > 0;
}

bool
Display::Blank(bool blank)
{
  RootDC dc;

  VIDEO_POWER_MANAGEMENT vpm;
  vpm.Length = sizeof(vpm);
  vpm.DPMSVersion = 0x0001;
  vpm.PowerState = blank ? VideoPowerOff : VideoPowerOn;

  return ExtEscape(dc, SETPOWERMANAGEMENT,
                   sizeof(vpm), (LPCSTR)&vpm, 0, NULL) > 0;
}

#endif /* HAVE_HARDWARE_BLANK */

#ifdef _WIN32_WCE

static bool
SetHP31XBacklight()
{
  const DWORD max_level = 20;
  const DWORD use_ext = 0;

  RegistryKey key(HKEY_CURRENT_USER, _T("ControlPanel\\Backlight"), false);
  return !key.error() &&
    key.set_value(_T("BackLightCurrentACLevel"), max_level) &&
    key.set_value(_T("BackLightCurrentBatteryLevel"), max_level) &&
    key.set_value(_T("TotalLevels"), max_level) &&
    key.set_value(_T("UseExt"), use_ext) &&
    key.delete_value(_T("ACTimeout")) &&
    TriggerGlobalEvent(_T("BacklightChangeEvent"));
}

/**
 * SetBacklight for PNA devices. There is no standard way of managing backlight on CE,
 * and every device may have different value names and settings. Microsoft did not set
 * a standard and thus we need a custom solution for each device.
 * But the approach is always the same: change a value and call an event.
 * We do this in XCSoar.cpp at the beginning, no need to make these settings configurable:
 * max brightness and no timeout if on power is the rule. Otherwise, do it manually..
 */
bool
Display::SetBacklight()
{
  switch (GlobalModelType) {
  case MODELTYPE_PNA_HP31X:
    return SetHP31XBacklight();

  default:
    return false;
  }
}
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
#elif defined(ANDROID)
  return true;
#else
  return false;
#endif
}

bool
Display::Rotate(enum orientation orientation)
{
#ifndef ANDROID
  if (orientation == ORIENTATION_DEFAULT)
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
  case ORIENTATION_PORTRAIT:
    DeviceMode.dmDisplayOrientation = native_landscape
      ? DMDO_90
      : initial_orientation;
    break;

  case ORIENTATION_LANDSCAPE:
    DeviceMode.dmDisplayOrientation = native_landscape
      ? initial_orientation
      : DMDO_270;
    break;

  case ORIENTATION_REVERSE_PORTRAIT:
    DeviceMode.dmDisplayOrientation = (native_landscape ? DMDO_270 : DMDO_180);
    break;

  case ORIENTATION_REVERSE_LANDSCAPE:
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

  NativeView::screen_orientation android_orientation;
  switch (orientation) {
  case ORIENTATION_PORTRAIT:
    android_orientation = NativeView::SCREEN_ORIENTATION_PORTRAIT;
    break;

  case ORIENTATION_LANDSCAPE:
    android_orientation = NativeView::SCREEN_ORIENTATION_LANDSCAPE;
    break;

  case ORIENTATION_REVERSE_PORTRAIT:
    android_orientation = model_is_galaxy_tab22() ?
                          NativeView::SCREEN_ORIENTATION_REVERSE_PORTRAIT_GT :
                          NativeView::SCREEN_ORIENTATION_REVERSE_PORTRAIT;
    break;

  case ORIENTATION_REVERSE_LANDSCAPE:
    android_orientation = model_is_galaxy_tab22() ?
                          NativeView::SCREEN_ORIENTATION_REVERSE_LANDSCAPE_GT :
                          NativeView::SCREEN_ORIENTATION_REVERSE_LANDSCAPE;
    break;

  default:
    android_orientation = NativeView::SCREEN_ORIENTATION_SENSOR;
  };

  return native_view->setRequestedOrientation(android_orientation);
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
  return native_view->setRequestedOrientation(NativeView::SCREEN_ORIENTATION_SENSOR);
#else
  return false;
#endif
}

int
Display::GetXDPI()
{
#ifdef WIN32
  RootDC dc;
  return GetDeviceCaps(dc, LOGPIXELSX);
#else
  return 96;
#endif
}

int
Display::GetYDPI()
{
#ifdef WIN32
  RootDC dc;
  return GetDeviceCaps(dc, LOGPIXELSY);
#else
  return 96;
#endif
}
