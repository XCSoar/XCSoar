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
#include "Interface.hpp"
#include "LogFile.hpp"
#include "UtilsSystem.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/MainWindow.hpp"
#include "ProcessTimer.hpp"
#include "Message.h"
#include "InfoBoxManager.h"
#include "ButtonLabel.h"

#include <commctrl.h>
#if (WINDOWSPC<1)
#include <sipapi.h>
#if !defined(CECORE) || UNDER_CE >= 300 || _WIN32_WCE >= 0x0300
#include <aygshell.h>
#endif
#endif

#include "Asset.hpp"
#include <assert.h>

///////////////////////////////////////////////////////////////////////////////
HINSTANCE hInst; // The current instance
MainWindow main_window;

////////////////////////////////////////////////////////////////////////////////
//Local Static data
int iTimerID= 0;


#if (((UNDER_CE >= 300)||(_WIN32_WCE >= 0x0300)) && (WINDOWSPC<1))
#define HAVE_ACTIVATE_INFO
#endif

#ifdef HAVE_ACTIVATE_INFO
static SHACTIVATEINFO s_sai;
#endif


WPARAM WindowsMessageLoop() {
#if _DEBUG
 // _crtBreakAlloc = -1;     // Set this to the number in {} brackets to
                             // break on a memory leak
#endif

  HACCEL hAccelTable = LoadAccelerators(hInst, (LPCTSTR)IDC_XCSOAR);

  // Main message loop:
  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0)) {
    if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

#if (WINDOWSPC>0)
#if _DEBUG
  _CrtCheckMemory();
  _CrtDumpMemoryLeaks();
#endif
#endif

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

  InitCommonControls();

  StartupStore(TEXT("Initialise application instance\n"));

  // Perform application initialization:
  if (!Startup (hInstance, lpCmdLine)) {
    return FALSE;
  }

  return WindowsMessageLoop();
}


///////////////////////////////////////////////////////////////////////////
// Windows event handlers

LRESULT MainHandler_Command(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  int wmId, wmEvent;
  HWND wmControl;

  wmId    = LOWORD(wParam);
  wmEvent = HIWORD(wParam);
  wmControl = (HWND)lParam;

  if(wmControl != NULL) {
    if (globalRunningEvent.test()) {

      main_window.full_screen();

      if (InfoBoxManager::Click(wmControl)) {
	return FALSE;
      }

      Message::CheckTouch(wmControl);

      if (ButtonLabel::CheckButtonPress(wmControl)) {
        return TRUE; // don't continue processing..
      }

    }
  }
  return DefWindowProc(hWnd, message, wParam, lParam);
}


LRESULT CALLBACK MainHandler_Colour(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  long wdata = Window::get_userdata((HWND)lParam);
  switch(wdata) {
  case 0:
    SetBkColor((HDC)wParam, MapGfx.ColorUnselected);
    SetTextColor((HDC)wParam, MapGfx.ColorBlack);
    return (LRESULT)MapGfx.infoUnselectedBrush.native();
  case 1:
    SetBkColor((HDC)wParam, MapGfx.ColorSelected);
    SetTextColor((HDC)wParam, MapGfx.ColorBlack);
    return (LRESULT)MapGfx.infoSelectedBrush.native();
  case 2:
    SetBkColor((HDC)wParam, MapGfx.ColorUnselected);
    SetTextColor((HDC)wParam, MapGfx.ColorWarning);
    return (LRESULT)MapGfx.infoUnselectedBrush.native();
  case 3:
    SetBkColor((HDC)wParam, MapGfx.ColorUnselected);
    SetTextColor((HDC)wParam, MapGfx.ColorOK);
    return (LRESULT)MapGfx.infoUnselectedBrush.native();
  case 4:
    // black on light green
    SetBkColor((HDC)wParam, MapGfx.ColorButton);
    SetTextColor((HDC)wParam, MapGfx.ColorBlack);
    return (LRESULT)MapGfx.buttonBrush.native();
  case 5:
    // grey on light green
    SetBkColor((HDC)wParam, MapGfx.ColorButton);
    SetTextColor((HDC)wParam, MapGfx.ColorMidGrey);
    return (LRESULT)MapGfx.buttonBrush.native();
  }
}

LRESULT CALLBACK MainHandler_Timer(void)
{
  if (globalRunningEvent.test()) {
    AfterStartup();
#ifdef _SIM_
    SIMProcessTimer();
#else
    ProcessTimer();
#endif
  }
}


LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message) {
    case WM_ERASEBKGND:
      return TRUE; // JMW trying to reduce screen flicker
      break;
    case WM_COMMAND:
      return MainHandler_Command(hWnd, message, wParam, lParam);
      break;
    case WM_CTLCOLORSTATIC:
      return MainHandler_Colour(hWnd, message, wParam, lParam);
      break;
    case WM_ACTIVATE:
      if(LOWORD(wParam) != WA_INACTIVE) {
	SetWindowPos(main_window,HWND_TOP, 0, 0, 0, 0,
		     SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOSIZE);
#ifdef HAVE_ACTIVATE_INFO
	SHFullScreen(main_window,SHFS_HIDETASKBAR|SHFS_HIDESIPBUTTON|SHFS_HIDESTARTICON);
#endif
      }
#ifdef HAVE_ACTIVATE_INFO
      SHHandleWMActivate(hWnd, wParam, lParam, &s_sai, FALSE);
#endif
      break;
    case WM_SETTINGCHANGE:
#ifdef HAVE_ACTIVATE_INFO
      SHHandleWMSettingChange(hWnd, wParam, lParam, &s_sai);
#endif
      break;
    case WM_SETFOCUS:
      // JMW not sure this ever does anything useful..
      if (globalRunningEvent.test()) {
	InfoBoxManager::Focus();
      }
      break;
#if defined(GNAV) || defined(PCGNAV)
    case WM_KEYDOWN: // JMW was keyup
#else
    case WM_KEYUP: // JMW was keyup
#endif
      InterfaceTimeoutReset();
      /* DON'T PROCESS KEYS HERE WITH NEWINFOBOX, IT CAUSES CRASHES! */
      break;
    case WM_TIMER:
      MainHandler_Timer();
      break;
    case WM_CREATE:
      main_window.created(hWnd);

#ifdef HAVE_ACTIVATE_INFO
      memset (&s_sai, 0, sizeof (s_sai));
      s_sai.cbSize = sizeof (s_sai);
#endif
      if (iTimerID == 0) {
        iTimerID = SetTimer(hWnd,1000,500,NULL); // 2 times per second
      }
      break;
    case WM_CLOSE:
      if (CheckShutdown()) {
	Shutdown();
      }
      break;
    case WM_DESTROY:
      if (hWnd==main_window) {
        PostQuitMessage(0);
      }
      break;
    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
    }
  return 0;
}


