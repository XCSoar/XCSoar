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
#include "Audio/Sound.hpp"
#include "LogFile.hpp"
#include "SettingsUser.hpp"
#include "SettingsComputer.hpp"
#include "Language.hpp"
#include "StatusMessage.hpp"

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

StatusMessageList Message::status_messages;

Mutex Message::mutexMessage;
RECT Message::rcmsg;
EditWindow Message::window;
struct singleMessage Message::messages[MAXMESSAGES];
bool Message::hidden=false;
int Message::nvisible=0;

TCHAR Message::msgText[2000];

// Get start time to reduce overrun errors
DWORD startTime = ::GetTickCount();

int Message::block_ref = 0;

void Message::Initialize(RECT rc) {
  block_ref = 0;

  rcmsg = rc; // default; message window can be full size of screen

  window.set_ro_ml(main_window, rcmsg.left, rcmsg.top,
                   rcmsg.right - rcmsg.left, rcmsg.bottom - rcmsg.top);
  window.set_font(MapWindowBoldFont);

  hidden = false;
  nvisible = 0;

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
  window.reset();
}


void Message::Lock() {
  mutexMessage.Lock();
}

void Message::Unlock() {
  mutexMessage.Unlock();
}



void Message::Resize() {
  int size = _tcslen(msgText);
  RECT rthis;
  //  RECT mRc;

  if (size==0) {
    if (!hidden) {
      window.hide();

      // animation
      //      GetWindowRect(hWndMessageWindow, &mRc);
      //      SetSourceRectangle(mRc);
      //      mRc.top=0; mRc.bottom=0;
      //      DrawWireRects(&mRc, 5);
    }
    hidden = true;
  } else {
    window.set_text(msgText);
    SIZE tsize = main_window.get_canvas().text_size(msgText);

    int linecount = max((unsigned)nvisible, max(1, window.get_row_count()));

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

    window.move(rthis.left, rthis.top,
                rthis.right - rthis.left,
                rthis.bottom - rthis.top);
    window.bring_to_top();
    window.show();
    hidden = false;
  }

}


void Message::BlockRender(bool doblock) {
  Lock();
  if (doblock) {
    block_ref++;
  } else {
    block_ref--;
  }
  // TODO code: add blocked time to messages' timers so they come
  // up once unblocked.
  Unlock();
}


bool Message::Render() {
  if (!globalRunningEvent.test()) return false;

  Lock();
  if (block_ref) return false;
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
  ScopeLock protect(mutexMessage);

  int i;
  DWORD	fpsTime = ::GetTickCount() - startTime;
  i = GetEmptySlot();

  messages[i].type = type;
  messages[i].tshow = tshow;
  messages[i].tstart = fpsTime;
  messages[i].texpiry = fpsTime;
  _tcscpy(messages[i].text, Text);

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
  if (wmControl == (HWND)window) {
    // acknowledge with click/touch
    Acknowledge(0);
  }
}


bool Message::Acknowledge(int type) {
  ScopeLock protect(mutexMessage);
  int i;
  DWORD	fpsTime = ::GetTickCount() - startTime;

  for (i=0; i<MAXMESSAGES; i++) {
    if ((messages[i].texpiry> messages[i].tstart)
	&& ((type==0)||(type==messages[i].type))) {
      // message was previously visible, so make it expire now.
      messages[i].texpiry = fpsTime-1;
	  return true;
    }
  }
  return false;
}

void Message::InitFile() {
}

void Message::LoadFile() {
  status_messages.LoadFile();
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
  ScopeLock protect(mutexMessage);

  StatusMessageSTRUCT LocalMessage = status_messages.First();
  const StatusMessageSTRUCT *found = status_messages.Find(text);
  if (found != NULL)
    LocalMessage = *found;

  if (SettingsComputer().EnableSoundModes && LocalMessage.doSound)
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
}


void Message::Startup(bool first) {
  status_messages.Startup(first);
}
