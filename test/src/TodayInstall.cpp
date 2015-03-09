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

#include "Config/Registry.hpp"
#include "Util/StringAPI.hpp"

#include <windows.h>
#include <tchar.h>

static bool
Uninstall()
{
  RegistryKey key(HKEY_LOCAL_MACHINE,
                  _T("\\Software\\Microsoft\\Today\\Items\\XCSoar"), false);
  return !key.error() && key.DeleteValue(_T("DLL"));
}

static bool
Install()
{
  RegistryKey key(HKEY_LOCAL_MACHINE,
                  _T("\\Software\\Microsoft\\Today\\Items\\XCSoar"), false);
  return !key.error() &&
    key.SetValue(_T("DLL"), _T("\\Windows\\XCSoarLaunch.dll"));
}

static bool
Enable()
{
  RegistryKey key(HKEY_LOCAL_MACHINE,
                  _T("\\Software\\Microsoft\\Today\\Items\\XCSoar"), false);
  return !key.error() &&
    key.SetValue(_T("Enabled"), DWORD(1));
}

static bool
Disable()
{
  RegistryKey key(HKEY_LOCAL_MACHINE,
                  _T("\\Software\\Microsoft\\Today\\Items\\XCSoar"), false);
  return !key.error() &&
    key.SetValue(_T("Enabled"), DWORD(0));
}

static bool
Do(LPCTSTR cmd)
{
  if (StringIsEqual(cmd, _T("uninstall")))
    return Disable() && Uninstall();
  else if (StringIsEqual(cmd, _T("enable")))
    return Enable();
  else if (StringIsEqual(cmd, _T("disable")))
    return Disable();
  else
    return Install();
}

int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
        LPTSTR lpCmdLine,
        int nCmdShow)
{
  if (Do(lpCmdLine)) {
    SendMessage(HWND_BROADCAST, WM_WININICHANGE, 0xF2, 0);
    return 0;
  } else {
    MessageBox(NULL, _T("Error"), NULL, MB_ICONEXCLAMATION|MB_OK);
    return 1;
  }
}
