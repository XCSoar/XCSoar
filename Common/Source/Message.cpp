/*
  
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
#include "Message.h"
#include "MapWindow.h"
#include "externs.h"
#include "InfoBoxLayout.h"


/*

  - Single window, created in GUI thread.
     -- hidden when no messages for display
     -- shown when messages available
     -- disappear when touched
     -- disappear when return clicked
     -- disappear when timeout
     -- disappear when extern event triggered
  - Message properties
     -- have a start time (seconds)
     -- timeout (start time + delta)
  - Messages stay in a circular buffer can be reviewed
  - Optional logging of all messages to file
  - Thread locking so available from any thread

*/

CRITICAL_SECTION  CritSec_Messages;

extern HWND hWndMainWindow; // Main Windows
extern HINSTANCE hInst;      // The current instance

RECT Message::rcmsg;
HWND Message::hWndMessageWindow;
HDC Message::hdc;
struct singleMessage Message::messages[MAXMESSAGES];
bool Message::hidden=false;
int Message::nvisible=0;

TCHAR Message::msgText[2000];
extern HFONT MapWindowBoldFont;

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
				   WS_VISIBLE|WS_CHILD|ES_MULTILINE|ES_CENTER
				   |WS_BORDER|ES_READONLY | WS_CLIPCHILDREN 
				   | WS_CLIPSIBLINGS,
				   0,0,0,0,hWndMainWindow,NULL,hInst,NULL);

  SetWindowPos(hWndMessageWindow, HWND_TOP,
	       rcmsg.left, rcmsg.top, 
	       rcmsg.right-rcmsg.left, rcmsg.bottom-rcmsg.top,
	       SWP_SHOWWINDOW);
  ShowWindow(hWndMessageWindow, SW_HIDE);

  SendMessage(hWndMessageWindow, WM_SETFONT,
		     (WPARAM)MapWindowBoldFont,MAKELPARAM(TRUE,0));

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

  if (size==0) {
    if (!hidden) {
      ShowWindow(hWndMessageWindow, SW_HIDE);
      MapWindow::RequestFastRefresh();      
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

    if (Appearance.StateMessageAlligne == smAlligneTopLeft){
      rthis.top = 0;
      rthis.left = 0;
      rthis.bottom = height;
      rthis.right = 206*InfoBoxLayout::scale; 
      // TODO: this shouldn't be hard-coded
    } else {
      rthis.left = midx-width/2;
      rthis.right = midx+width/2;
      rthis.top = midy-h1;
      rthis.bottom = midy+h2;
    }

    SetWindowPos(hWndMessageWindow, HWND_TOP,
		 rthis.left, rthis.top,
		 rthis.right-rthis.left, 
		 rthis.bottom-rthis.top,
		 SWP_SHOWWINDOW);
    hidden = false;

    // window has resized potentially, so redraw map to reduce artifacts
    MapWindow::RequestFastRefresh();

  }

}


void Message::BlockRender(bool doblock) {
  //  Lock();
  if (doblock) {
    block_ref++;
  } else {
    block_ref--;
  }
  // TODO: add blocked time to messages' timers so they come
  // up once unblocked.
  //  Unlock();
}


void Message::Render() {
  DWORD	fpsTime = ::GetTickCount() - startTime;

  if (!GlobalRunning) return;

  if (block_ref) return;

  Lock();

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
    Unlock(); return; 
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
  Render();
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
  int i;
  bool ret = false;	// Did we acknowledge?

  Lock();
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
  Render();
  return ret;
}

