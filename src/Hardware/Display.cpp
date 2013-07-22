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

#include "Display.hpp"

#ifdef WIN32
#include "Asset.hpp"
#include "Config/Registry.hpp"
#include "OS/GlobalEvent.hpp"

#include <windows.h>
#endif

#ifdef _WIN32_WCE

static bool
SetHP31XBacklight()
{
  const DWORD max_level = 20;
  const DWORD use_ext = 0;

  RegistryKey key(HKEY_CURRENT_USER, _T("ControlPanel\\Backlight"), false);
  return !key.error() &&
    key.SetValue(_T("BackLightCurrentACLevel"), max_level) &&
    key.SetValue(_T("BackLightCurrentBatteryLevel"), max_level) &&
    key.SetValue(_T("TotalLevels"), max_level) &&
    key.SetValue(_T("UseExt"), use_ext) &&
    key.DeleteValue(_T("ACTimeout")) &&
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
  switch (global_model_type) {
  case ModelType::HP31X:
    return SetHP31XBacklight();

  default:
    return false;
  }
}
#endif
