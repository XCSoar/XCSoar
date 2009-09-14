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

#include "Dialogs.h"
#include "Language.hpp"
#include "XCSoar.h"
#include "Version.hpp"
#include "LogFile.hpp"
#include "MapWindowProjection.hpp"
#include "MainWindow.hpp"
#include "Waypointparser.h"
#include "SettingsTask.hpp"
#include "Airspace.h"

#include <commdlg.h>
#include <commctrl.h>

#ifndef WINDOWSPC
#include "aygshell.h"
#endif

#include "UtilsText.hpp"
#include "UtilsSystem.hpp"
#include "LocalPath.hpp"
#include "Settings.hpp"
#include "SettingsUser.hpp"
#include "InfoBoxLayout.h"
#include "InputEvents.h"
#include "Message.h"
#include "Registry.hpp"
#include "Interface.hpp"

///////////////////////////////////

LRESULT CALLBACK Progress(HWND hDlg, UINT message,
                          WPARAM wParam, LPARAM lParam)
{
  PAINTSTRUCT ps;            // structure for paint info
  HDC hDC;
  RECT rc;
  (void)lParam;
  switch (message)
    {
    case WM_INITDIALOG:
#ifdef WINDOWSPC
      GetClientRect(hDlg, &rc);
      MoveWindow(hDlg, 0, 0, rc.right-rc.left, rc.bottom-rc.top, TRUE);
#endif
      return TRUE;
    case WM_ERASEBKGND:
      hDC = BeginPaint(hDlg, &ps);
      SelectObject(hDC, GetStockObject(WHITE_PEN));
      SelectObject(hDC, GetStockObject(WHITE_BRUSH));
      GetClientRect(hDlg, &rc);
      Rectangle(hDC, rc.left,rc.top,rc.right,rc.bottom);
      DeleteDC(hDC);
      EndPaint(hDlg, &ps);
      return TRUE;
    case WM_COMMAND:
      if (LOWORD(wParam) == IDOK)
        {
          EndDialog(hDlg, LOWORD(wParam));
          return TRUE;
        }
      break;
    }
  return FALSE;
}

HWND XCSoarInterface::hProgress = NULL;
HWND XCSoarInterface::hWndCurtain = NULL;

HCURSOR ActionInterface::oldCursor = NULL;

void ActionInterface::StartHourglassCursor(void) {
  HCURSOR newc = LoadCursor(NULL, IDC_WAIT);
  oldCursor = (HCURSOR)SetCursor(newc);
#ifdef GNAV
  SetCursorPos(160,120);
#endif
}

void ActionInterface::StopHourglassCursor(void) {
  SetCursor(oldCursor);
#ifdef GNAV
  SetCursorPos(640,480);
#endif
  oldCursor = NULL;
}

void XCSoarInterface::CloseProgressDialog() {
  if (hProgress) {
    DestroyWindow(hProgress);
    hProgress = NULL;
  }
  if (hWndCurtain) {
    DestroyWindow(hWndCurtain);
    hWndCurtain = NULL;
  }
}

void XCSoarInterface::StepProgressDialog(void) {
  if (hProgress) {
    SendMessage(GetDlgItem(hProgress, IDC_PROGRESS1), PBM_STEPIT,
		(WPARAM)0, (LPARAM)0);
    UpdateWindow(hProgress);
  }
}

BOOL XCSoarInterface::SetProgressStepSize(int nSize) {
  nSize = 5;
  if (hProgress)
    if (nSize < 100)
      SendMessage(GetDlgItem(hProgress, IDC_PROGRESS1),
		  PBM_SETSTEP, (WPARAM)nSize, (LPARAM)0);
  return(TRUE);
}


HWND XCSoarInterface::CreateProgressDialog(const TCHAR* text) {
  if (hProgress) {
  } else {
    if (InfoBoxLayout::landscape) {
      hProgress=
	CreateDialog(hInst,
		     (LPCTSTR)IDD_PROGRESS_LANDSCAPE,
		     main_window,
		     (DLGPROC)Progress);

    } else {
      hProgress=
	CreateDialog(hInst,
		     (LPCTSTR)IDD_PROGRESS,
		     main_window,
		     (DLGPROC)Progress);
    }

    TCHAR Temp[1024];
    _stprintf(Temp,TEXT("%s %s"),gettext(TEXT("Version")),XCSoar_Version);
    SetWindowText(GetDlgItem(hProgress,IDC_VERSION),Temp);

    RECT rc;
    GetClientRect(main_window, &rc);

#ifndef WINDOWSPC
    hWndCurtain = CreateWindow(TEXT("STATIC"), TEXT(" "),
			       WS_VISIBLE | WS_CHILD,
                               0, 0, (rc.right - rc.left),
			       (rc.bottom-rc.top),
                               main_window, NULL, hInst, NULL);
    SetWindowPos(hWndCurtain,HWND_TOP,0,0,0,0,
                 SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
    ShowWindow(hWndCurtain,SW_SHOW);
    SetForegroundWindow(hWndCurtain);
    UpdateWindow(hWndCurtain);
#endif

#ifdef WINDOWSPC
    RECT rcp;
    GetClientRect(hProgress, &rcp);
    GetWindowRect(main_window, &rc);
    SetWindowPos(hProgress,HWND_TOP,
                 rc.left, rc.top, (rcp.right - rcp.left), (rcp.bottom-rcp.top),
                 SWP_SHOWWINDOW);
#else
#ifndef GNAV
    SHFullScreen(hProgress,
		 SHFS_HIDETASKBAR
		 |SHFS_HIDESIPBUTTON
		 |SHFS_HIDESTARTICON);
#endif
    SetWindowPos(hProgress,HWND_TOP,0,0,0,0,
                 SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
#endif

    SendMessage(GetDlgItem(hProgress, IDC_PROGRESS1),
		PBM_SETRANGE, (WPARAM)0,
		(LPARAM) MAKELPARAM (0, 100));
    SendMessage(GetDlgItem(hProgress, IDC_PROGRESS1),
		PBM_SETSTEP, (WPARAM)5, (LPARAM)0);

    SetForegroundWindow(hProgress);
    UpdateWindow(hProgress);
  }

  SetDlgItemText(hProgress,IDC_MESSAGE, text);
  SendMessage(GetDlgItem(hProgress, IDC_PROGRESS1), PBM_SETPOS, 0, 0);
  UpdateWindow(hProgress);
  return hProgress;
}


///////////////

void PopupAnalysis()
{
  dlgAnalysisShowModal();
}


void PopupWaypointDetails()
{
  dlgWayPointDetailsShowModal();
}


#include "Interface.hpp"
#include "MapWindow.h"

bool PopupNearestWaypointDetails(const GEOPOINT &location,
                                 double range,
                                 bool pan) 
{
  /*
    if (!pan) {
    dlgWayPointSelect(lon, lat, 0, 1);
    } else {
    dlgWayPointSelect(PanLongitude, PanLatitude, 0, 1);
    }
  */
  MapWindowProjection &map_window = XCSoarInterface::main_window.map;

  int i;
  if (!pan || !XCSoarInterface::SettingsMap().EnablePan) {
    i=FindNearestWayPoint(map_window, location, range);
  } else {
    // nearest to center of screen if in pan mode
    i=FindNearestWayPoint(map_window, 
			  map_window.GetPanLocation(),
			  range);
  }
  if(i != -1) {
    SelectedWaypoint = i;
    PopupWaypointDetails();
    return true;
  }

  return false;
}


bool PopupInteriorAirspaceDetails(const GEOPOINT &location) {
  unsigned int i;
  bool found=false;
  bool inside;

  if (AirspaceCircle) {
    for (i=0; i<NumberOfAirspaceCircles; i++) {
      inside = false;
      if (AirspaceCircle[i].Visible) {
        inside = InsideAirspaceCircle(location, i);
      }
      if (inside) {
	dlgAirspaceDetails(i, -1);
        found = true;
      }
    }
  }
  if (AirspaceArea) {
    for (i=0; i<NumberOfAirspaceAreas; i++) {
      inside = false;
      if (AirspaceArea[i].Visible) {
        inside = InsideAirspaceArea(location, i);
      }
      if (inside) {
	dlgAirspaceDetails(-1, i);
        found = true;
      }
    }
  }

  return found; // nothing found..
}
