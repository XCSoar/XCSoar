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

/**
 * Defines the entry point for the DLL application.
 * @file XCSoarSetup.cpp
 * @see http://www.codeguru.com/Cpp/W-P/ce/networking/article.php/c9269/
 */

#include <windows.h>

#ifdef __GNUC__
/* cegcc doesn't provide ce_setup.h; the following enum values are
   taken from MSDN */
typedef enum {
  codeINSTALL_INIT_CONTINUE = 0,
  codeINSTALL_INIT_CANCEL
} codeINSTALL_INIT;

typedef enum {
  codeINSTALL_EXIT_DONE = 0,
  codeINSTALL_EXIT_UNINSTALL
} codeINSTALL_EXIT;

typedef enum {
  codeUNINSTALL_INIT_CONTINUE = 0,
  codeUNINSTALL_INIT_CANCEL
} codeUNINSTALL_INIT;

typedef enum {
  codeUNINSTALL_EXIT_DONE = 0
} codeUNINSTALL_EXIT;

/* disable C++ name mangling for exported functions */
extern "C" {
  DECLSPEC_EXPORT codeINSTALL_INIT
  Install_Init(HWND hwndparent, BOOL ffirstcall, BOOL fpreviouslyinstalled,
               LPCTSTR pszinstalldir);

  DECLSPEC_EXPORT codeINSTALL_EXIT
  Install_Exit(HWND hwndparent, LPCTSTR pszinstalldir,
               WORD cfaileddirs, WORD cfailedfiles, WORD cfailedregkeys,
               WORD cfailedregvals, WORD cfailedshortcuts);

  DECLSPEC_EXPORT codeUNINSTALL_INIT
  Uninstall_Init(HWND hwndparent, LPCTSTR pszinstalldir);

  DECLSPEC_EXPORT codeUNINSTALL_EXIT
  Uninstall_Exit(HWND hwndparent);

  DECLSPEC_EXPORT APIENTRY BOOL
  DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved);
};

#else
#if (WIN32_PLATFORM_PSPC == 1)
  #include <..\support\ActiveSync\inc\ce_setup.h>
#else
  #include <ce_setup.h>
#endif
#endif

/**
 * Handles tasks done at start of installation
 */
codeINSTALL_INIT
Install_Init(HWND hwndparent, BOOL ffirstcall, BOOL fpreviouslyinstalled,
             LPCTSTR pszinstalldir)
{
  HKEY hKey = NULL;

  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                   TEXT("\\Software\\Microsoft\\Today\\Items\\XCSoar"),
                   0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) {
    RegDeleteValue(hKey, TEXT("DLL"));
    SendMessage(HWND_BROADCAST, WM_WININICHANGE, 0xF2, 0);
    RegCloseKey(hKey);
  }

  return codeINSTALL_INIT_CONTINUE;
}

/**
 * Handles tasks done at end of installation
 */
codeINSTALL_EXIT
Install_Exit(HWND hwndparent, LPCTSTR pszinstalldir, WORD cfaileddirs,
             WORD cfailedfiles, WORD cfailedregkeys, WORD cfailedregvals,
             WORD cfailedshortcuts)
{
  SendMessage(HWND_BROADCAST, WM_WININICHANGE, 0xF2, 0);
  return codeINSTALL_EXIT_DONE;
}

/**
 * Handles tasks done at beginning of uninstallation
 */
codeUNINSTALL_INIT
Uninstall_Init(HWND hwndparent, LPCTSTR pszinstalldir)
{
  HKEY hKey = NULL;

  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                   TEXT("\\Software\\Microsoft\\Today\\Items\\XCSoar"),
                   0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) {
    RegDeleteValue(hKey, TEXT("DLL"));
    RegCloseKey(hKey);
    SendMessage(HWND_BROADCAST, WM_WININICHANGE, 0xF2, 0);
  }

  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                   TEXT("\\Software\\Microsoft\\Today\\Items"),
                   0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) {
    RegDeleteKey(hKey, TEXT("XCSoar"));
    RegCloseKey(hKey);
  }

  return codeUNINSTALL_INIT_CONTINUE;
}

/**
 * Handles tasks done at end of uninstallation
 */
codeUNINSTALL_EXIT
Uninstall_Exit(HWND hwndparent)
{
  return codeUNINSTALL_EXIT_DONE;
}

BOOL APIENTRY
DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
  return TRUE;
}
