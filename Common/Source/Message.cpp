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

#include "Message.h"
#include "Interface.hpp"
#include "Protection.hpp"
#include "InfoBoxLayout.h"
#include "Screen/Fonts.hpp"
#include "MainWindow.hpp"
#include "LocalPath.hpp"
#include "Registry.hpp"
#include <tchar.h>
#include <stdio.h>
#include "UtilsText.hpp"
#include "UtilsSystem.hpp"
#include "LogFile.hpp"
#include "SettingsUser.hpp"
#include "SettingsComputer.hpp"
#include "Language.hpp"
/*

  - Single window, created in GUI thread.
     -- hidden when no messages for display
     -- shown when messages available
     -- disappear when touched
     -- disappear when return clicked
     -- disappear when timeout
     -- disappear when extrn event triggered
  - Message properties
     -- have a start time (seconds)
     -- timeout (start time + delta)
  - Messages stay in a circular buffer can be reviewed
  - Optional logging of all messages to file
  - Thread locking so available from any thread

*/

CRITICAL_SECTION  CritSec_Messages;

RECT Message::rcmsg;
HWND Message::hWndMessageWindow;
HDC Message::hdc;
struct singleMessage Message::messages[MAXMESSAGES];
bool Message::hidden=false;
int Message::nvisible=0;

TCHAR Message::msgText[2000];

// Get start time to reduce overrun errors
DWORD startTime = ::GetTickCount();

// Intercept messages destined for the Status Message window
LRESULT CALLBACK MessageWindowProc(HWND hwnd, UINT message,
				   WPARAM wParam, LPARAM lParam)
{

  POINT pt;
  RECT  rc;
  static bool capturedMouse = false;

  switch (message) {
  case WM_LBUTTONDOWN:

    // Intercept mouse messages while stylus is being dragged
    // This is necessary to simulate a WM_LBUTTONCLK event
    SetCapture(hwnd);
    capturedMouse = TRUE;

    return 0;
  case WM_LBUTTONUP :

    ReleaseCapture();
    if (!capturedMouse) return 0;
    capturedMouse = FALSE;

    // Is stylus still within this window?
    pt.x = LOWORD(lParam);
    pt.y = HIWORD(lParam);
    GetClientRect(hwnd, &rc);

    if (!PtInRect(&rc, pt)) return 0;

    DestroyWindow(hwnd);
    return 0;
  }
  return DefWindowProc(hwnd, message, wParam, lParam);
}


int Message::block_ref = 0;

void Message::Initialize(RECT rc) {

  InitializeCriticalSection(&CritSec_Messages);

  block_ref = 0;

  rcmsg = rc; // default; message window can be full size of screen

  hWndMessageWindow = CreateWindow(// WS_EX_CLIENTEDGE,
				     TEXT("EDIT"), TEXT(" "),
				   WS_CHILD|ES_MULTILINE|ES_CENTER
				   |WS_BORDER|ES_READONLY | WS_CLIPCHILDREN
				   | WS_CLIPSIBLINGS,
				     0,0,0,0, 
				     main_window, NULL, 
				     hInst, NULL);

  SetWindowPos(hWndMessageWindow, HWND_TOP,
	       rcmsg.left, rcmsg.top,
	       rcmsg.right-rcmsg.left, rcmsg.bottom-rcmsg.top,
	       SWP_HIDEWINDOW);

  SendMessage(hWndMessageWindow, WM_SETFONT,
	      (WPARAM)MapWindowBoldFont.native(),MAKELPARAM(TRUE,0));

  /*
  SetWindowLong(hWndMessageWindow, GWL_WNDPROC,
		(LONG) MessageWindowProc);
  EnableWindow(hWndMessageWindow, FALSE); // prevent window receiving
					  // keyboard/mouse input
  */

  hdc = GetDC(hWndMessageWindow);

  hidden = false;
  nvisible = 0;

  //  for (x=0; TabStops[x] != 0 && x < 10; x++);
  //  SendMessage(hWnd, EM_SETTABSTOPS, (WPARAM)x, (LPARAM)TabStops);

  int i;
  for (i=0; i<MAXMESSAGES; i++) {
    messages[i].text[0]= _T('\0');
    messages[i].tstart = 0;
    messages[i].texpiry = 0;
    messages[i].type = 0;
  }

}


void Message::Destroy() {
  // destroy window
  ReleaseDC(hWndMessageWindow, hdc);
  DestroyWindow(hWndMessageWindow);
  DeleteCriticalSection(&CritSec_Messages);
}


void Message::Lock() {
  EnterCriticalSection(&CritSec_Messages);
}

void Message::Unlock() {
  LeaveCriticalSection(&CritSec_Messages);
}



void Message::Resize() {
  SIZE tsize;
  int size = _tcslen(msgText);
  RECT rthis;
  //  RECT mRc;

  if (size==0) {
    if (!hidden) {
      ShowWindow(hWndMessageWindow, SW_HIDE);

      // animation
      //      GetWindowRect(hWndMessageWindow, &mRc);
      //      SetSourceRectangle(mRc);
      //      mRc.top=0; mRc.bottom=0;
      //      DrawWireRects(&mRc, 5);
    }
    hidden = true;
  } else {
    SetWindowText(hWndMessageWindow, msgText);
    GetTextExtentPoint(hdc, msgText, size, &tsize);

    int linecount = max(nvisible,max(1,
			SendMessage(hWndMessageWindow,
				    EM_GETLINECOUNT, 0, 0)));

    int width =// min((rcmsg.right-rcmsg.left)*0.8,tsize.cx);
      (int)((rcmsg.right-rcmsg.left)*0.9);
    int height = (int)min((rcmsg.bottom-rcmsg.top)*0.8,tsize.cy*(linecount+1));
    int h1 = height/2;
    int h2 = height-h1;

    int midx = (rcmsg.right+rcmsg.left)/2;
    int midy = (rcmsg.bottom+rcmsg.top)/2;

    if (Appearance.StateMessageAlign == smAlignTopLeft){
      rthis.top = 0;
      rthis.left = 0;
      rthis.bottom = height;
      rthis.right = 206*InfoBoxLayout::scale;
      // TODO code: this shouldn't be hard-coded
    } else {
      rthis.left = midx-width/2;
      rthis.right = midx+width/2;
      rthis.top = midy-h1;
      rthis.bottom = midy+h2;
    }
    /*
    if (hidden) {
      RECT bigrect;
      GetWindowRect(hWndMapWindow, &bigrect);
      GetWindowRect(hWndMessageWindow, &mRc);
      bigrect.bottom= bigrect.top;
      SetSourceRectangle(mRc);
      DrawWireRects(&mRc, 10);
    }
    */

    SetWindowPos(hWndMessageWindow, HWND_TOP,
		 rthis.left, rthis.top,
		 rthis.right-rthis.left,
		 rthis.bottom-rthis.top,
		 SWP_SHOWWINDOW);
    hidden = false;
  }

}


void Message::BlockRender(bool doblock) {
  //Lock();
  if (doblock) {
    block_ref++;
  } else {
    block_ref--;
  }
  // TODO code: add blocked time to messages' timers so they come
  // up once unblocked.
  //Unlock();
}


bool Message::Render() {
  if (!globalRunningEvent.test()) return false;
  if (block_ref) return false;

  Lock();
  DWORD	fpsTime = ::GetTickCount() - startTime;

  // this has to be done quickly, since it happens in GUI thread
  // at subsecond interval

  // first loop through all messages, and determine which should be
  // made invisible that were previously visible, or
  // new messages

  bool changed = false;
  int i;
  for (i=0; i<MAXMESSAGES; i++) {
    if (messages[i].type==0) continue; // ignore unknown messages

    if (
	(messages[i].texpiry <= fpsTime)
	&&(messages[i].texpiry> messages[i].tstart)
	) {
      // this message has expired for first time
      changed = true;
      continue;
    }

    // new message has been added
    if (messages[i].texpiry== messages[i].tstart) {
      // set new expiry time.
      messages[i].texpiry = fpsTime + messages[i].tshow;
      // this is a new message..
      changed = true;
    }

  }

  static bool doresize= false;

  if (!changed) {
    if (doresize) {
      doresize = false;
      // do one extra resize after display so we are sure we get all
      // the text (workaround bug in getlinecount)
      Resize();
    }
    Unlock(); return false;
  }

  // ok, we've changed the visible messages, so need to regenerate the
  // text box

  doresize = true;
  msgText[0]= 0;
  nvisible=0;
  for (i=0; i<MAXMESSAGES; i++) {
    if (messages[i].type==0) continue; // ignore unknown messages

    if (messages[i].texpiry< fpsTime) {
      messages[i].texpiry = messages[i].tstart-1;
      // reset expiry so we don't refresh
      continue;
    }

    _tcscat(msgText, messages[i].text);
    _tcscat(msgText, TEXT("\r\n"));
    nvisible++;

  }
  Resize();

  Unlock();
  return true;
}



int Message::GetEmptySlot() {
  // find oldest message that is no longer visible

  // todo: make this more robust with respect to message types and if can't
  // find anything to remove..
  int i;
  DWORD tmin=0;
  int imin=0;
  for (i=0; i<MAXMESSAGES; i++) {
    if ((i==0) || (messages[i].tstart<tmin)) {
      tmin = messages[i].tstart;
      imin = i;
    }
  }
  return imin;
}


void Message::AddMessage(DWORD tshow, int type, TCHAR* Text) {

  Lock();

  int i;
  DWORD	fpsTime = ::GetTickCount() - startTime;
  i = GetEmptySlot();

  messages[i].type = type;
  messages[i].tshow = tshow;
  messages[i].tstart = fpsTime;
  messages[i].texpiry = fpsTime;
  _tcscpy(messages[i].text, Text);

  Unlock();
  //  Render(); // NO this causes crashes (don't know why..)
}

void Message::Repeat(int type) {
  int i;
  DWORD tmax=0;
  int imax= -1;

  Lock();

  DWORD	fpsTime = ::GetTickCount() - startTime;

  // find most recent non-visible message

  for (i=0; i<MAXMESSAGES; i++) {

    if ((messages[i].texpiry < fpsTime)
	&&(messages[i].tstart > tmax)
	&&((messages[i].type == type)|| (type==0))) {
      imax = i;
      tmax = messages[i].tstart;
    }

  }

  if (imax>=0) {
    messages[imax].tstart = fpsTime;
    messages[imax].texpiry = messages[imax].tstart;
  }

  Unlock();
}


void Message::CheckTouch(HWND wmControl) {
  if (wmControl == hWndMessageWindow) {
    // acknowledge with click/touch
    Acknowledge(0);
  }
}


bool Message::Acknowledge(int type) {
  Lock();
  int i;
  bool ret = false;	// Did we acknowledge?
  DWORD	fpsTime = ::GetTickCount() - startTime;

  for (i=0; i<MAXMESSAGES; i++) {
    if ((messages[i].texpiry> messages[i].tstart)
	&& ((type==0)||(type==messages[i].type))) {
      // message was previously visible, so make it expire now.
      messages[i].texpiry = fpsTime-1;
	  ret = true;
    }
  }

  Unlock();
  //  Render(); NO! this can cause crashes
  return ret;
}



typedef struct {
	const TCHAR *key;		/* English key */
	const TCHAR *sound;		/* What sound entry to play */
	const TCHAR *nmea_gps;		/* NMEA Sentence - to GPS serial */
	const TCHAR *nmea_vario;		/* NMEA Sentence - to Vario serial */
	bool doStatus;
	bool doSound;
	int delay_ms;		/* Delay for DoStatusMessage */
	int iFontHeightRatio;	// TODO - not yet used
	bool docenter;		// TODO - not yet used
	int *TabStops;		// TODO - not yet used
	int disabled;		/* Disabled - currently during run time */
} StatusMessageSTRUCT;

void _init_Status(int num);


StatusMessageSTRUCT StatusMessageData[MAXSTATUSMESSAGECACHE];
int StatusMessageData_Size = 0;

void Message::InitFile() {
  StartupStore(TEXT("StatusFileInit\n"));

  // DEFAULT - 0 is loaded as default, and assumed to exist
  StatusMessageData[0].key = TEXT("DEFAULT");
  StatusMessageData[0].doStatus = true;
  StatusMessageData[0].doSound = true;
  StatusMessageData[0].sound = TEXT("IDR_WAV_DRIP");
  StatusMessageData_Size=1;
#ifdef VENTA_DEBUG_EVENT // VENTA- longer statusmessage delay in event debug mode
	StatusMessageData[0].delay_ms = 10000;  // 10 s
#else
    StatusMessageData[0].delay_ms = 2500; // 2.5 s
#endif

  // Load up other defaults - allow overwrite in config file
#include "Status_defaults.cpp"

}

void Message::LoadFile() {

  StartupStore(TEXT("Loading status file\n"));

  TCHAR szFile1[MAX_PATH] = TEXT("\0");
  FILE *fp=NULL;

  // Open file from registry
  GetRegistryString(szRegistryStatusFile, szFile1, MAX_PATH);
  ExpandLocalPath(szFile1);

  SetRegistryString(szRegistryStatusFile, TEXT("\0"));

  if (_tcslen(szFile1)>0)
    fp  = _tfopen(szFile1, TEXT("rt"));

  // Unable to open file
  if (fp == NULL)
    return;

  // TODO code: Safer sizes, strings etc - use C++ (can scanf restrict length?)
  TCHAR buffer[2049];	// Buffer for all
  TCHAR key[2049];	// key from scanf
  TCHAR value[2049];	// value from scanf
  int ms;				// Found ms for delay
  const TCHAR **location;	// Where to put the data
  int found;			// Entries found from scanf
  bool some_data;		// Did we find some in the last loop...

  // Init first entry
  _init_Status(StatusMessageData_Size);
  some_data = false;

  /* Read from the file */
  while (
	 (StatusMessageData_Size < MAXSTATUSMESSAGECACHE)
	 && _fgetts(buffer, 2048, fp)
	 && ((found = _stscanf(buffer, TEXT("%[^#=]=%[^\n]\n"), key, value)) != EOF)
	 ) {
    // Check valid line? If not valid, assume next record (primative, but works ok!)
    if ((found != 2) || !key || !value) {

      // Global counter (only if the last entry had some data)
      if (some_data) {
	StatusMessageData_Size++;
	some_data = false;
	_init_Status(StatusMessageData_Size);
      }

    } else {

      location = NULL;

      if (_tcscmp(key, TEXT("key")) == 0) {
	some_data = true;	// Success, we have a real entry
	location = &StatusMessageData[StatusMessageData_Size].key;
      } else if (_tcscmp(key, TEXT("sound")) == 0) {
	StatusMessageData[StatusMessageData_Size].doSound = true;
	location = &StatusMessageData[StatusMessageData_Size].sound;
      } else if (_tcscmp(key, TEXT("delay")) == 0) {
	if (_stscanf(value, TEXT("%d"), &ms) == 1)
	  StatusMessageData[StatusMessageData_Size].delay_ms = ms;
      } else if (_tcscmp(key, TEXT("hide")) == 0) {
	if (_tcscmp(value, TEXT("yes")) == 0)
	  StatusMessageData[StatusMessageData_Size].doStatus = false;
      }

      // Do we have somewhere to put this && is it currently empty ? (prevent lost at startup)
      if (location && (_tcscmp(*location, TEXT("")) == 0)) {
	// TODO code: this picks up memory lost from no entry, but not duplicates - fix.
	if (*location) {
	  // JMW fix memory leak
          free((void*)*location);
	}
	*location = StringMallocParse(value);
      }
    }

  }

  // How many we really got (blank next just in case)
  StatusMessageData_Size++;
  _init_Status(StatusMessageData_Size);

  // file was ok, so save it to registry
  ContractLocalPath(szFile1);
  SetRegistryString(szRegistryStatusFile, szFile1);

  fclose(fp);
}


// Create a blank entry (not actually used)
void _init_Status(int num) {
  StatusMessageData[num].key = TEXT("");
  StatusMessageData[num].doStatus = true;
  StatusMessageData[num].doSound = false;
  StatusMessageData[num].sound = TEXT("");
  StatusMessageData[num].delay_ms = 2500;  // 2.5 s
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
// TODO code: (need to discuss) Consider moving almost all this functionality into AddMessage ?

void Message::AddMessage(const TCHAR* text, const TCHAR *data) {
  Lock();

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

    AddMessage(LocalMessage.delay_ms, MSG_USERINTERFACE, msgcache);
  }

  Unlock();
}


void Message::Startup(bool first) {
  static int olddelay = 2000;
  if (first) {
    // NOTE: Must show errors AFTER all windows ready
    olddelay = StatusMessageData[0].delay_ms;
    StatusMessageData[0].delay_ms = 20000; // 20 seconds
  } else {
    StatusMessageData[0].delay_ms = olddelay;
  }
}
