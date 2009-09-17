/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>

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

#include "XCSoar.h"
#include "Version.hpp"
#include "Protection.hpp"
#include "Components.hpp"
#include "LogFile.hpp"
#include "UtilsSystem.hpp"
#include "MainWindow.hpp"
#include "Message.h"
#include "Asset.hpp"
#include "Interface.hpp"

#ifndef ENABLE_SDL
#include <commctrl.h>
#ifndef WINDOWSPC
#if !defined(CECORE) || UNDER_CE >= 300 || _WIN32_WCE >= 0x0300
#include <aygshell.h>
#endif
#endif
#endif /* !ENABLE_SDL */

///////////////////////////////////////////////////////////////////////////////

// Main message loop
WPARAM WindowsMessageLoop(HINSTANCE hInstance) 
{
  HACCEL hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_XCSOAR);
  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0)) {
    if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
  return msg.wParam;
}



int WINAPI WinMain(     HINSTANCE hInstance,
                        HINSTANCE hPrevInstance,
                        LPTSTR    lpCmdLine,
                        int       nCmdShow)
{
  (void)hPrevInstance;

  InitAsset();

  Version();
  StartupStore(TEXT("Starting XCSoar %s\n"), XCSoar_Version);

  XCSoarGetOpts(lpCmdLine);

#ifndef ENABLE_SDL
  InitCommonControls();
#endif /* !ENABLE_SDL */

  StartupStore(TEXT("Initialise application instance\n"));

  // Perform application initialization and run loop
  if (XCSoarInterface::Startup (hInstance, lpCmdLine)) {
    return WindowsMessageLoop(hInstance);
  } else {
    return FALSE;
  }
}


/////////////////////////////////////////


/*
#if _DEBUG
 // _crtBreakAlloc = -1;     // Set this to the number in {} brackets to
                             // break on a memory leak
#endif
#ifdef WINDOWSPC
#if _DEBUG
  _CrtCheckMemory();
  _CrtDumpMemoryLeaks();
#endif
#endif

*/
