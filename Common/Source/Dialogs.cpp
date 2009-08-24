/*

  $Id$

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
#include "XCSoar.h"
#include "LogFile.hpp"

#include <commdlg.h>
#include <commctrl.h>

#if !WINDOWSPC
#include "aygshell.h"
#endif

#include "Logger.h"
#include "Utils.h"
#include "LocalPath.hpp"
#include "externs.h"
#include "Device/Port.h"
#include "AirfieldDetails.h"
#include "Device/device.h"
#include "Units.h"
#include "InfoBoxLayout.h"
#include "InputEvents.h"
#include "Message.h"
#include "Registry.hpp"
#include "StatusMessage.hpp"

#ifdef DEBUG_TRANSLATIONS
#include <map>
#endif

static GetTextSTRUCT GetTextData[MAXSTATUSMESSAGECACHE];
static int GetTextData_Size = 0;

void ReadWayPoints(void);
void ReadAirspace(void);
int FindIndex(HWND hWnd);
void ReadNewTask(HWND hDlg);

const TCHAR *PolarLabels[] = {TEXT("Vintage - Ka6"),
			      TEXT("Club - ASW19"),
			      TEXT("Standard - LS8"),
			      TEXT("15M - ASW27"),
			      TEXT("18M - LS6C"),
			      TEXT("Open - ASW22"),
			      TEXT("WinPilot File")};


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
#if (WINDOWSPC>0)
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


#ifdef ENABLE_UNUSED_CODE
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
#endif /* ENABLE_UNUSED_CODE */


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
// TODO code: (need to discuss) Consider moving almost all this functionality into AddMessage ?

void DoStatusMessage(const TCHAR* text, const TCHAR *data) {
  Message::Lock();

  StatusMessageSTRUCT LocalMessage;
  LocalMessage = StatusMessageData[0];

  int i;
  // Search from end of list (allow overwrites by user)
  for (i=StatusMessageData_Size - 1; i>0; i--) {
    if (_tcscmp(text, StatusMessageData[i].key) == 0) {
      LocalMessage = StatusMessageData[i];
      break;
    }
  }

  if (EnableSoundModes && LocalMessage.doSound)
    PlayResource(LocalMessage.sound);

  // TODO code: consider what is a sensible size?
  TCHAR msgcache[1024];
  if (LocalMessage.doStatus) {

    _tcscpy(msgcache, gettext(text));
    if (data != NULL) {
      _tcscat(msgcache, TEXT(" "));
      _tcscat(msgcache, data);
    }

    Message::AddMessage(LocalMessage.delay_ms, 1, msgcache);
  }

  Message::Unlock();
}


#ifdef DEBUG_TRANSLATIONS
/*

  WriteMissingTranslations - write all missing translations found
  during runtime to a lanuage file in data dir

*/
template<class _Ty>
struct lessTCHAR: public std::binary_function<_Ty, _Ty, bool>
{	// functor for operator<
  bool operator()(const _Ty& _Left, const _Ty& _Right) const
  {	// apply operator< to operands
    return (_tcscmp(_Left, _Right) < 0);
  }
};

std::map<TCHAR*, TCHAR*, lessTCHAR<TCHAR*> > unusedTranslations;

void WriteMissingTranslations() {
  std::map<TCHAR*, TCHAR*, lessTCHAR<TCHAR*> >::iterator
    s=unusedTranslations.begin(),e=unusedTranslations.end();

  TCHAR szFile1[MAX_PATH] = TEXT("%LOCAL_PATH%\\\\localization_todo.xcl\0");
  FILE *fp=NULL;

  ExpandLocalPath(szFile1);
  fp  = _tfopen(szFile1, TEXT("w+"));

  if (fp != NULL) {
    while (s != e) {
      TCHAR* p = (s->second);
      if (p) {
        while (*p) {
          if (*p != _T('\n')) {
            fwprintf(fp, TEXT("%c"), *p);
          } else {
            fwprintf(fp, TEXT("\\n"));
          }
          p++;
        }
        fwprintf(fp, TEXT("=\n"));
      }
      s++;
    }
    fclose(fp);
  }
}

#endif




/*

  gettext - look up a string of text for the current language

  Currently very simple. Looks up the current string and current language
  to find the appropriate string response. On failure will return the string itself.

  NOTES CACHING:
  	- Could load the whole file or part
	- qsort/bsearch good idea
	- cache misses in data structure for future use

   TODO enhancement: Fast search of text strings

*/

const TCHAR* gettext(const TCHAR* text) {
  int i;
  // return if nothing to do
  if (_tcscmp(text, _T("")) == 0) return (const TCHAR*)text;

  //find a translation
  for (i=0; i<GetTextData_Size; i++) {
    if (!text || !GetTextData[i].key) continue;
    if (_tcscmp(text, GetTextData[i].key) == 0)
      return GetTextData[i].text;
  }

  // return untranslated text if no translation is found.
  // Set a breakpoint here to find untranslated strings

#ifdef DEBUG_TRANSLATIONS
  TCHAR *tmp = _tcsdup(text);
  unusedTranslations[tmp] = tmp;
#endif
  return (const TCHAR*)text;
}

// Set the window text value, by passing through gettext
// TODO code: Review if this needs to be done every time?
void SetWindowText_gettext(HWND hDlg, int entry) {
  TCHAR strTemp[1024];
  GetWindowText(GetDlgItem(hDlg,entry),strTemp, 1023);
  SetWindowText(GetDlgItem(hDlg,entry),gettext(strTemp));
}


void ReadLanguageFile() {
  StartupStore(TEXT("Loading language file\n"));

  TCHAR szFile1[MAX_PATH] = TEXT("\0");
  FILE *fp=NULL;

  // Open file from registry
  GetRegistryString(szRegistryLanguageFile, szFile1, MAX_PATH);
  ExpandLocalPath(szFile1);

  SetRegistryString(szRegistryLanguageFile, TEXT("\0"));

  if (_tcslen(szFile1)==0) {
    // JMW set default language file if none present
    _tcscpy(szFile1,TEXT("default.xcl"));
  }

  fp  = _tfopen(szFile1, TEXT("rt"));

  if (fp == NULL)
    return;

  // TODO code: Safer sizes, strings etc - use C++ (can scanf restrict length?)
  TCHAR buffer[2049];	// key from scanf
  TCHAR key[2049];	// key from scanf
  TCHAR value[2049];	// value from scanf
  int found;            // Entries found from scanf

  /* Read from the file */
  while (
  	 (GetTextData_Size < MAXSTATUSMESSAGECACHE)
	 && _fgetts(buffer, 2048, fp)
	 && ((found = _stscanf(buffer, TEXT("%[^#=]=%[^\r\n][\r\n]"), key, value)) != EOF)
         ) {
    // Check valid line?
    if ((found != 2) || !key || !value) continue;

    GetTextData[GetTextData_Size].key = StringMallocParse(key);
    GetTextData[GetTextData_Size].text = StringMallocParse(value);

    // Global counter
    GetTextData_Size++;
  }

  // file was OK, so save registry
  ContractLocalPath(szFile1);
  SetRegistryString(szRegistryLanguageFile, szFile1);

  fclose(fp);
}


/////////////////////////////////////////////////

static HWND hProgress = NULL;
static HWND hWndCurtain = NULL;

static HCURSOR oldCursor = NULL;

void StartHourglassCursor(void) {
  HCURSOR newc = LoadCursor(NULL, IDC_WAIT);
  oldCursor = (HCURSOR)SetCursor(newc);
#ifdef GNAV
  SetCursorPos(160,120);
#endif
}

void StopHourglassCursor(void) {
  SetCursor(oldCursor);
#ifdef GNAV
  SetCursorPos(640,480);
#endif
  oldCursor = NULL;
}

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

void StepProgressDialog(void) {
  if (hProgress) {
    SendMessage(GetDlgItem(hProgress, IDC_PROGRESS1), PBM_STEPIT,
		(WPARAM)0, (LPARAM)0);
    UpdateWindow(hProgress);
  }
}

BOOL SetProgressStepSize(int nSize) {
  nSize = 5;
  if (hProgress)
    if (nSize < 100)
      SendMessage(GetDlgItem(hProgress, IDC_PROGRESS1),
		  PBM_SETSTEP, (WPARAM)nSize, (LPARAM)0);
  return(TRUE);
}


HWND CreateProgressDialog(const TCHAR* text) {
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

    RECT rc;
    GetClientRect(hWndMainWindow, &rc);

#if (WINDOWSPC<1)
    hWndCurtain = CreateWindow(TEXT("STATIC"), TEXT(" "),
			       WS_VISIBLE | WS_CHILD,
                               0, 0, (rc.right - rc.left),
			       (rc.bottom-rc.top),
                               hWndMainWindow, NULL, hInst, NULL);
    SetWindowPos(hWndCurtain,HWND_TOP,0,0,0,0,
                 SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
    ShowWindow(hWndCurtain,SW_SHOW);
    SetForegroundWindow(hWndCurtain);
    UpdateWindow(hWndCurtain);
#endif

#if (WINDOWSPC>0)
    RECT rcp;
    GetClientRect(hProgress, &rcp);
    GetWindowRect(hWndMainWindow, &rc);
    SetWindowPos(hProgress,HWND_TOP,
                 rc.left, rc.top, (rcp.right - rcp.left), (rcp.bottom-rcp.top),
                 SWP_SHOWWINDOW);
#else
    SHFullScreen(hProgress,
		 SHFS_HIDETASKBAR
		 |SHFS_HIDESIPBUTTON
		 |SHFS_HIDESTARTICON);
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


