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

#include "PopupMessage.hpp"
#include "Protection.hpp"
#include "Screen/AnyCanvas.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/SingleWindow.hpp"
#include "Screen/Layout.hpp"
#include "LocalPath.hpp"
#include "UtilsText.hpp"
#include "Audio/Sound.hpp"
#include "LogFile.hpp"
#include "SettingsComputer.hpp"
#include "Appearance.hpp"
#include "Language.hpp"
#include "StatusMessage.hpp"

#include <tchar.h>
#include <stdio.h>
#include <algorithm>

using std::min;
using std::max;

void
PopupMessage::singleMessage::Set(int _type, int _tshow, const TCHAR *_text,
                                 int now)
{
  type = _type;
  tshow = _tshow;
  tstart = now;
  texpiry = now;
  _tcscpy(text, _text);
}

bool
PopupMessage::singleMessage::Update(int now)
{
  if (IsUnknown())
    // ignore unknown messages
    return false;

  if (IsNewlyExpired(now))
    // this message has expired for first time
    return true;

  // new message has been added
  if (IsNew()) {
    // set new expiry time.
    texpiry = now + tshow;
    // this is a new message..
    return true;
  }

  return false;
}

bool
PopupMessage::singleMessage::AppendTo(TCHAR *buffer, int now)
{
  if (IsUnknown())
    // ignore unknown messages
    return false;

  if (texpiry < now) {
    texpiry = tstart - 1;
    // reset expiry so we don't refresh
    return false;
  }

  if (buffer[0] != _T('\0'))
    _tcscat(buffer, _T("\r\n"));
  _tcscat(buffer, text);
  return true;
}

PopupMessage::PopupMessage(const StatusMessageList &_status_messages,
                           SingleWindow &_parent)
  :status_messages(_status_messages),
   parent(_parent),
   nvisible(0),
   enable_sound(true)
{
  clock.update();
}

void
PopupMessage::set(const PixelRect _rc)
{
  rc = _rc;

  EditWindowStyle style;
  style.border();
  style.center();
  style.multiline();
  style.read_only();
  style.hide();

  EditWindow::set(parent, rc.left, rc.top,
                  rc.right - rc.left, rc.bottom - rc.top,
                  style);

  set_font(Fonts::MapBold);
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
  PixelRect rthis;

  if (*msgText == _T('\0')) {
    hide();
  } else {
    set_text(msgText);

    AnyCanvas canvas;
    canvas.select(Fonts::MapBold);
    PixelSize tsize = canvas.text_size(msgText);

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

    move(rthis.left, rthis.top,
         rthis.right - rthis.left,
         rthis.bottom - rthis.top);
    show_on_top();
  }

}

bool PopupMessage::Render() {
  if (!globalRunningEvent.test()) return false;

  mutex.Lock();
  if (parent.has_dialog()) {
    mutex.Unlock();
    return false;
  }

  int fpsTime = clock.elapsed();

  // this has to be done quickly, since it happens in GUI thread
  // at subsecond interval

  // first loop through all messages, and determine which should be
  // made invisible that were previously visible, or
  // new messages

  bool changed = false;
  for (unsigned i = 0; i < MAXMESSAGES; ++i)
    changed = messages[i].Update(fpsTime) || changed;

  static bool doresize= false;

  if (!changed) {
    mutex.Unlock();

    if (doresize) {
      doresize = false;
      // do one extra resize after display so we are sure we get all
      // the text (workaround bug in getlinecount)
      Resize();
    }
    return false;
  }

  // ok, we've changed the visible messages, so need to regenerate the
  // text box

  doresize = true;
  msgText[0]= _T('\0');
  nvisible=0;
  for (unsigned i = 0; i < MAXMESSAGES; ++i)
    if (messages[i].AppendTo(msgText, fpsTime))
      nvisible++;

  mutex.Unlock();

  Resize();

  return true;
}



int PopupMessage::GetEmptySlot() {
  // find oldest message that is no longer visible

  // todo: make this more robust with respect to message types and if can't
  // find anything to remove..
  int i;
  int tmin=0;
  int imin=0;
  for (i=0; i<MAXMESSAGES; i++) {
    if ((i==0) || (messages[i].tstart<tmin)) {
      tmin = messages[i].tstart;
      imin = i;
    }
  }
  return imin;
}


void
PopupMessage::AddMessage(int tshow, int type, const TCHAR *Text)
{
  ScopeLock protect(mutex);

  int i;
  int fpsTime = clock.elapsed();
  i = GetEmptySlot();

  messages[i].Set(type, tshow, Text, fpsTime);
}

void PopupMessage::Repeat(int type) {
  int i;
  int tmax=0;
  int imax= -1;

  mutex.Lock();

  int fpsTime = clock.elapsed();

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

  mutex.Unlock();
}

bool PopupMessage::Acknowledge(int type) {
  ScopeLock protect(mutex);
  int i;
  int fpsTime = clock.elapsed();

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

  if (enable_sound && LocalMessage.doSound)
    PlayResource(LocalMessage.sound);

  // TODO code: consider what is a sensible size?
  TCHAR msgcache[1024];
  if (LocalMessage.doStatus) {

    _tcscpy(msgcache, text);
    if (data != NULL) {
      _tcscat(msgcache, _T(" "));
      _tcscat(msgcache, data);
    }

    AddMessage(LocalMessage.delay_ms, MSG_USERINTERFACE, msgcache);
  }
}
