/*

Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

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
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "PopupMessage.hpp"
#include "Protection.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/ContainerWindow.hpp"
#include "Screen/Layout.hpp"
#include "LocalPath.hpp"
#include "Registry.hpp"
#include "UtilsText.hpp"
#include "Audio/Sound.hpp"
#include "LogFile.hpp"
#include "SettingsComputer.hpp"
#include "Appearance.hpp"
#include "Language.hpp"
#include "StatusMessage.hpp"

#include <tchar.h>
#include <stdio.h>

#ifndef _MSC_VER
#include <algorithm>
using std::min;
using std::max;
#endif

PopupMessage::PopupMessage(const StatusMessageList &_status_messages,
                           ContainerWindow &_parent)
  :startTime(::GetTickCount()), status_messages(_status_messages),
   parent(_parent), hidden(true),
   nvisible(0), block_ref(0)
{
  for (unsigned i = 0; i < MAXMESSAGES; i++) {
    messages[i].text[0]= _T('\0');
    messages[i].tstart = 0;
    messages[i].texpiry = 0;
    messages[i].type = 0;
  }
}

void
PopupMessage::set(const RECT _rc)
{
  rc = _rc;
  set_ro_ml(parent, rc.left, rc.top,
            rc.right - rc.left, rc.bottom - rc.top);
  set_font(MapWindowBoldFont);
  install_wndproc();
}

bool
PopupMessage::on_mouse_down(int x, int y)
{
  // acknowledge with click/touch
  Acknowledge(0);

  return true;
}

void PopupMessage::Resize() {
  int size = _tcslen(msgText);
  RECT rthis;
  //  RECT mRc;

  if (size==0) {
    if (!hidden) {
      Unlock();
      hide();
      Lock();

      // animation
      //      GetWindowRect(hWndMessageWindow, &mRc);
      //      SetSourceRectangle(mRc);
      //      mRc.top=0; mRc.bottom=0;
      //      DrawWireRects(&mRc, 5);
    }
    hidden = true;
  } else {
    Unlock();

    set_text(msgText);

    SIZE tsize = parent.get_canvas().text_size(msgText);

    int linecount = max((unsigned)nvisible, max((unsigned)1, get_row_count()));

    int width =// min((rc.right-rc.left)*0.8,tsize.cx);
      (int)((rc.right-rc.left)*0.9);
    int height = min((int)((rc.bottom-rc.top) * 0.8),
                     (int)tsize.cy * (linecount + 1));
    int h1 = height/2;
    int h2 = height-h1;

    int midx = (rc.right+rc.left)/2;
    int midy = (rc.bottom+rc.top)/2;

    if (Appearance.StateMessageAlign == smAlignTopLeft){
      rthis.top = 0;
      rthis.left = 0;
      rthis.bottom = height;
      rthis.right = Layout::FastScale(206);
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

    move(rthis.left, rthis.top,
         rthis.right - rthis.left,
         rthis.bottom - rthis.top);
    bring_to_top();
    show();

    Lock();

    hidden = false;
  }

}


void PopupMessage::BlockRender(bool doblock) {
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


bool PopupMessage::Render() {
  if (!globalRunningEvent.test()) return false;

  Lock();
  if (block_ref) {
    Unlock();
    return false;
  }

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



int PopupMessage::GetEmptySlot() {
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


void PopupMessage::AddMessage(DWORD tshow, int type, TCHAR* Text) {
  ScopeLock protect(mutex);

  int i;
  DWORD	fpsTime = ::GetTickCount() - startTime;
  i = GetEmptySlot();

  messages[i].type = type;
  messages[i].tshow = tshow;
  messages[i].tstart = fpsTime;
  messages[i].texpiry = fpsTime;
  _tcscpy(messages[i].text, Text);

}

void PopupMessage::Repeat(int type) {
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

bool PopupMessage::Acknowledge(int type) {
  ScopeLock protect(mutex);
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

void PopupMessage::AddMessage(const TCHAR* text, const TCHAR *data) {
  ScopeLock protect(mutex);

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
