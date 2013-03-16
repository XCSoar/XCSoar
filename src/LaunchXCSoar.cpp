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

/**
 * This is a program launcher for Windows CE, to be used as an AutoRun
 * binary.  It searches for XCSoar.exe and executes it.
 *
 * This has been tested on the Naviter Oudie 2.  Copy this program to
 * the Oudie's storage (right next to XCSoar.exe) and rename it to
 * OudiePocketFMSStart.exe.
 */

#include "OS/PathName.hpp"
#include "Compiler.h"

#include <assert.h>
#include <windows.h>
#include <tchar.h>
#include <stdio.h>

static bool
LaunchExe(const TCHAR *exe, const TCHAR *cmdline)
{
  TCHAR buffer[256];
  _sntprintf(buffer, 256, _T("\"%s\" %s"), exe, cmdline);

  PROCESS_INFORMATION pi;
  if (!CreateProcess(exe, buffer, nullptr, nullptr, false,
                     0, nullptr, nullptr, nullptr, &pi))
    return false;

  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  return true;
}

static bool
FindAndLaunchXCSoar(const TCHAR *cmdline)
{
  TCHAR buffer[MAX_PATH];
  if (GetModuleFileName(nullptr, buffer, MAX_PATH) <= 0)
    return false;

  ReplaceBaseName(buffer, _T("XCSoar.exe"));
  return LaunchExe(buffer, cmdline);
}

int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
#ifdef _WIN32_WCE
        gcc_unused LPWSTR lpCmdLine,
#else
        gcc_unused LPSTR lpCmdLine2,
#endif
        int nCmdShow)
{
  return FindAndLaunchXCSoar(_T("-fly")) ? 0 : 1;
}
