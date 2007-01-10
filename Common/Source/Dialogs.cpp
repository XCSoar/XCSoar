/*

  $Id$

Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2005

  	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@bigfoot.com>

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

#include "stdafx.h"

#include <commdlg.h>
#include <commctrl.h>
#include "aygshell.h"

#include "compatibility.h"

#include "Dialogs.h"
#include "Logger.h"
#include "resource.h"
#include "utils.h"
#include "externs.h"
#include "Port.h"
#include "McReady.h"
#include "AirfieldDetails.h"
#include "VarioSound.h"
#include "device.h"
#include "units.h"
#include "GaugeVario.h"
#include "InfoBoxLayout.h"
#include "InputEvents.h"
#include "Message.h"


void ReadWayPoints(void);
void ReadAirspace(void);
int FindIndex(HWND hWnd);
void ReadNewTask(HWND hDlg);


static Task_t TaskBackup;
static BOOL fTaskModified = FALSE;


TCHAR *PolarLabels[] = {TEXT("Vintage - Ka6"),
			TEXT("Club - ASW19"),
			TEXT("Standard - LS8"),
			TEXT("15M - ASW27"),
			TEXT("18M - LS6C"),
			TEXT("Open - ASW22"),
			TEXT("WinPilot File")};


LRESULT CALLBACK Progress(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  PAINTSTRUCT ps;            // structure for paint info
  HDC hDC;
  RECT rc;
  (void)lParam;
  switch (message)
    {
    case WM_INITDIALOG:
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


// ARH: Status Message functions
// Used to show a brief status message to the user
// Could be used to display debug messages
// or info messages like "Map panning OFF"
/////////////////////////////////////////////////////

// Each instance of the StatusMessage window has some
// unique data associated with it, rather than using
// global variables.  This allows multiple instances
// in a thread-safe manner.
class CStatMsgUserData {
public:
  HFONT   hFont;
  WNDPROC fnOldWndProc;
  BOOL    bCapturedMouse;
  DWORD   texpiry; //

  // Initialize to sensible values
  CStatMsgUserData()
    : hFont(NULL), fnOldWndProc(NULL), bCapturedMouse(FALSE), texpiry(0) {};

  // Clean up mess
  ~CStatMsgUserData() {
    if (hFont) {
      DeleteObject(hFont);
      hFont = NULL;
    }
    fnOldWndProc = NULL;
  }
};


bool forceDestroyStatusMessage = false;

void ClearStatusMessages(void) {
  forceDestroyStatusMessage = true;
}


// Intercept messages destined for the Status Message window
LRESULT CALLBACK StatusMsgWndTimerProc(HWND hwnd, UINT message,
				       WPARAM wParam, LPARAM lParam)
{

  CStatMsgUserData *data;
  POINT pt;
  RECT  rc;

  // Grab hold of window specific data
  data = (CStatMsgUserData*) GetWindowLong(hwnd, GWL_USERDATA);

/*
  if (data==NULL) {
    // Something wrong here!
    DestroyWindow(hwnd);  // ups
    return 1;
  }
*/

  switch (message) {
  case WM_LBUTTONDOWN:

    if (data==NULL) {
      // Something wrong here!
      DestroyWindow(hwnd);  // ups
      return 1;
    }

    // Intercept mouse messages while stylus is being dragged
    // This is necessary to simulate a WM_LBUTTONCLK event
    SetCapture(hwnd);
    data->bCapturedMouse = TRUE;
    return 0;
  case WM_LBUTTONUP :

    if (data==NULL) {
      // Something wrong here!
      DestroyWindow(hwnd);  // ups
      return 1;
    }

    //if (data->bCapturedMouse) ReleaseCapture();
    ReleaseCapture();

    if (!data->bCapturedMouse) return 0;

    data->bCapturedMouse = FALSE;

    // Is stylus still within this window?
    pt.x = LOWORD(lParam);
    pt.y = HIWORD(lParam);
    GetClientRect(hwnd, &rc);

    if (!PtInRect(&rc, pt)) return 0;

    DestroyWindow(hwnd);
    return 0;

  case WM_TIMER :

    // force destruction of window...
    if (forceDestroyStatusMessage) {
      DestroyWindow(hwnd);
      return 0;
    }

    if ((data->texpiry>0) && (::GetTickCount()>data->texpiry)) {
      DestroyWindow(hwnd);
    }
    return 0;
  case WM_DESTROY :

    // Clean up after ourselves
    if (data != NULL){
      delete data;
      // hack ... try to find execption point
      data = NULL;
      // Attach window specific data to the window
      SetWindowLong(hwnd, GWL_USERDATA, (LONG) data);
    }
    MapWindow::RequestFastRefresh();

    ClearAirspaceWarnings(false);
    // JMW do this so airspace warning gets refreshed

    return 0;
  }

  // Pass message on to original window proc
  if (data != NULL)
    return CallWindowProc(data->fnOldWndProc, hwnd, message, wParam, lParam);
  else
    return(0);

}


// DoMessage is designed to delegate what to do for a message
// The "what to do" can be defined in a configuration file
// Defaults for each message include:
//	- Text to display (including multiple languages)
//	- Text to display extra - NOT multiple language
//		(eg: If Airspace Warning - what details - airfield name is in data file, already
//		covers multiple languages).
//	- ShowStatusMessage - including font size and delay
//	- Sound to play - What sound to play
//	- Log - Keep the message on the log/history window (goes to log file and history)
//
// TODO (need to discuss) Consider moving almost all this functionality into AddMessage ?

void DoStatusMessage(TCHAR* text, TCHAR *data) {
  Message::Lock();

	StatusMessageSTRUCT LocalMessage;
	LocalMessage = StatusMessageData[0];

	int i;
	// Search from end of list (allow overwrites by user)
	for (i=StatusMessageData_Size - 1; i>0; i--) {
		if (wcscmp(text, StatusMessageData[i].key) == 0) {
			LocalMessage = StatusMessageData[i];
			break;
		}
	}

	if (EnableSoundModes && LocalMessage.doSound)
		PlayResource(LocalMessage.sound);

	// TODO consider what is a sensible size?
	TCHAR msgcache[1024];
	if (LocalMessage.doStatus) {

	  wcscpy(msgcache, gettext(text));
	  if (data != NULL) {
	    wcscat(msgcache, TEXT(" "));
	    wcscat(msgcache, data);
	  }

	  Message::AddMessage(LocalMessage.delay_ms, 1, msgcache);
	}

  Message::Unlock();
}

/*

  gettext - look up a string of text for the current language

  Currently very simple. Looks up the current string and current language
  to find the appropriate string response. On failure will return the string itself.

  NOTES CACHING:
  	- Could load the whole file or part
	- qsort/bsearch good idea
	- cache misses in data structure for future use

   TODO - Fast search

*/

TCHAR* gettext(TCHAR* text) {
  int i;
  for (i=0; i<GetTextData_Size; i++) {
    if (!text || !GetTextData[i].key) continue;
    if (wcscmp(text, GetTextData[i].key) == 0)
      return GetTextData[i].text;
  }
  return text;
}

// Set the window text value, by passing through gettext
// TODO - Review if this needs to be done every time?
void SetWindowText_gettext(HWND hDlg, int entry) {
  TCHAR strTemp[1024];
  GetWindowText(GetDlgItem(hDlg,entry),strTemp, 1023);
  SetWindowText(GetDlgItem(hDlg,entry),gettext(strTemp));
}

/////////////////////////////////////////////////

static HWND hProgress = NULL;
static HWND hWndCurtain = NULL;

void CloseProgressDialog() {
  if (hProgress) {
    DestroyWindow(hProgress);
    hProgress = NULL;
  }
  if (hWndCurtain) {
    DestroyWindow(hWndCurtain);
    hWndCurtain = NULL;
  }
}

BOOL StepProgressDialog(void) {
  if (hProgress) {
    SendMessage(GetDlgItem(hProgress, IDC_PROGRESS1), PBM_STEPIT, 0, 0);
    UpdateWindow(hProgress);
  }
  return(TRUE);
}

BOOL SetProgressStepSize(int nSize) {
  if (hProgress)
    if (nSize < 100)
      SendMessage(GetDlgItem(hProgress, IDC_PROGRESS1),
		  PBM_SETSTEP, (WPARAM)nSize, 0);
  return(TRUE);
}

HWND CreateProgressDialog(TCHAR* text) {
#if (WINDOWSPC>2)
  hProgress = NULL;
  return NULL;
#endif
  if (hProgress) {
  } else {

    if (InfoBoxLayout::landscape) {
      hProgress=
	CreateDialog(hInst,
		     (LPCTSTR)IDD_PROGRESS_LANDSCAPE,
		     hWndMainWindow,
		     (DLGPROC)Progress);

    } else {
      hProgress=
	CreateDialog(hInst,
		     (LPCTSTR)IDD_PROGRESS,
		     hWndMainWindow,
		     (DLGPROC)Progress);
    }

    TCHAR Temp[1024];
    _stprintf(Temp,TEXT("%s %s"),gettext(TEXT("Version")),XCSoar_Version);
    SetWindowText(GetDlgItem(hProgress,IDC_VERSION),Temp);

    ShowWindow(hProgress,SW_SHOW);

    RECT rc;
    GetClientRect(hWndMainWindow, &rc);
    hWndCurtain = CreateWindow(TEXT("STATIC"), TEXT(" "),
			       WS_VISIBLE | WS_CHILD,
                               0, 0, (rc.right - rc.left),
			       (rc.bottom-rc.top),
                               hWndMainWindow, NULL, hInst, NULL);
    SetWindowPos(hWndCurtain,HWND_TOPMOST,0,0,0,0,
                 SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
    ShowWindow(hWndCurtain,SW_SHOW);
    SetForegroundWindow(hWndCurtain);
    UpdateWindow(hWndCurtain);

    SetForegroundWindow(hProgress);

#if (WINDOWSPC>0)
    SetWindowPos(hProgress,HWND_TOPMOST,
                 0, 0, 0, 0,
                 SWP_SHOWWINDOW);
#else
    SHFullScreen(hProgress,
		 SHFS_HIDETASKBAR
		 |SHFS_HIDESIPBUTTON
		 |SHFS_HIDESTARTICON);
    SetWindowPos(hProgress,HWND_TOPMOST,0,0,0,0,
                 SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
#endif

  }

  SetDlgItemText(hProgress,IDC_MESSAGE, text);
  SendMessage(GetDlgItem(hProgress, IDC_PROGRESS1), PBM_SETPOS, 0, 0);
  UpdateWindow(hProgress);
  return hProgress;
}


