/*

Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Look/Fonts.hpp"
#include "Screen/SingleWindow.hpp"
#include "Screen/Layout.hpp"
#include "LocalPath.hpp"
#include "Audio/Sound.hpp"
#include "LogFile.hpp"
#include "ComputerSettings.hpp"
#include "Language/Language.hpp"
#include "StatusMessage.hpp"
#include "UISettings.hpp"
#include "OS/Clock.hpp"

#include <tchar.h>
#include <stdio.h>
#include <algorithm>

using std::min;
using std::max;

void
PopupMessage::Message::Set(Type _type, unsigned _tshow, const TCHAR *_text,
                           unsigned now)
{
  type = _type;
  tshow = _tshow;
  tstart = now;
  texpiry = now;
  text = _text;
}

bool
PopupMessage::Message::Update(unsigned now)
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
PopupMessage::Message::AppendTo(StaticString<2000> &buffer, unsigned now)
{
  if (IsUnknown())
    // ignore unknown messages
    return false;

  if (texpiry < now) {
    texpiry = tstart - 1;
    // reset expiry so we don't refresh
    return false;
  }

  if (!buffer.empty())
    buffer.append(_T("\r\n"));
  buffer.append(text);
  return true;
}

PopupMessage::PopupMessage(const StatusMessageList &_status_messages,
                           SingleWindow &_parent, const UISettings &_settings)
  :status_messages(_status_messages),
   parent(_parent),
   settings(_settings),
   n_visible(0),
   enable_sound(true)
{
}

void
PopupMessage::Create(const PixelRect _rc)
{
  rc = _rc;

  LargeTextWindowStyle style;
  style.Border();
  style.SetCenter();
  style.Hide();

  LargeTextWindow::Create(parent, GetRect(100), style);

  SetFont(Fonts::map_bold);
  InstallWndProc();
}

bool
PopupMessage::OnMouseDown(PixelScalar x, PixelScalar y)
{
  // acknowledge with click/touch
  Acknowledge(MSG_UNKNOWN);

  return true;
}

PixelRect
PopupMessage::GetRect(UPixelScalar height) const
{
  PixelRect rthis;

  if (settings.popup_message_position == UISettings::smAlignTopLeft){
    rthis.top = 0;
    rthis.left = 0;
    rthis.bottom = height;
    rthis.right = Layout::FastScale(206);
    // TODO code: this shouldn't be hard-coded
  } else {
    PixelScalar width =// min((rc.right-rc.left)*0.8,tsize.cx);
      (PixelScalar)((rc.right - rc.left) * 0.9);
    PixelScalar midx = (rc.right + rc.left) / 2;
    PixelScalar midy = (rc.bottom + rc.top) / 2;
    PixelScalar h1 = height / 2;
    PixelScalar h2 = height - h1;
    rthis.left = midx-width/2;
    rthis.right = midx+width/2;
    rthis.top = midy-h1;
    rthis.bottom = midy+h2;
  }

  return rthis;
}

void
PopupMessage::UpdateTextAndLayout(const TCHAR *text)
{
  if (StringIsEmpty(text)) {
    Hide();
  } else {
    SetText(text);

    const UPixelScalar font_height = Fonts::map_bold.GetHeight();

    unsigned n_lines = max(n_visible, max(1u, GetRowCount()));

    PixelScalar height = min((PixelScalar)((rc.bottom-rc.top) * 0.8),
                             (PixelScalar)(font_height * (n_lines + 1)));

    PixelRect rthis = GetRect(height);
#ifdef USE_GDI
    PixelRect old_rc = GetPosition();
    if (rthis.left != old_rc.left || rthis.right != old_rc.right) {
      /* on Windows, the TEXT control can never change its text style
         after it has been created, so we have to destroy it and
         create a new one */
      Destroy();
      Create(rthis);
      SetText(text);
    } else
#endif
      Move(rthis);

    ShowOnTop();
  }
}

bool
PopupMessage::Render()
{
  mutex.Lock();
  if (parent.HasDialog()) {
    mutex.Unlock();
    return false;
  }

  const unsigned now = MonotonicClockMS();

  // this has to be done quickly, since it happens in GUI thread
  // at subsecond interval

  // first loop through all messages, and determine which should be
  // made invisible that were previously visible, or
  // new messages

  bool changed = false;
  for (unsigned i = 0; i < MAXMESSAGES; ++i)
    changed = messages[i].Update(now) || changed;

  static bool doresize = false;

  if (!changed) {
    mutex.Unlock();

    if (doresize) {
      doresize = false;
      // do one extra resize after display so we are sure we get all
      // the text (workaround bug in getlinecount)
      UpdateTextAndLayout(text);
    }
    return false;
  }

  // ok, we've changed the visible messages, so need to regenerate the
  // text box

  doresize = true;
  text.clear();
  n_visible = 0;
  for (unsigned i = 0; i < MAXMESSAGES; ++i)
    if (messages[i].AppendTo(text, now))
      n_visible++;

  mutex.Unlock();

  UpdateTextAndLayout(text);

  return true;
}

int
PopupMessage::GetEmptySlot()
{
  // find oldest message that is no longer visible

  // todo: make this more robust with respect to message types and if can't
  // find anything to remove..
  unsigned imin = 0;
  for (unsigned i = 0, tmin = 0; i < MAXMESSAGES; i++) {
    if (i == 0 || messages[i].tstart < tmin) {
      tmin = messages[i].tstart;
      imin = i;
    }
  }
  return imin;
}

void
PopupMessage::AddMessage(unsigned tshow, Type type, const TCHAR *Text)
{
  assert(mutex.IsLockedByCurrent());

  const unsigned now = MonotonicClockMS();

  int i = GetEmptySlot();
  messages[i].Set(type, tshow, Text, now);
}

void
PopupMessage::Repeat(Type type)
{
  int imax = -1;

  mutex.Lock();
  const unsigned now = MonotonicClockMS();

  // find most recent non-visible message

  for (unsigned i = 0, tmax = 0; i < MAXMESSAGES; i++) {
    if (messages[i].texpiry < now &&
        messages[i].tstart > tmax &&
        (messages[i].type == type || type == 0)) {
      imax = i;
      tmax = messages[i].tstart;
    }
  }

  if (imax >= 0) {
    messages[imax].tstart = now;
    messages[imax].texpiry = messages[imax].tstart;
  }

  mutex.Unlock();
}

bool
PopupMessage::Acknowledge(Type type)
{
  ScopeLock protect(mutex);
  const unsigned now = MonotonicClockMS();

  for (unsigned i = 0; i < MAXMESSAGES; i++) {
    if (messages[i].texpiry > messages[i].tstart &&
        (type == MSG_UNKNOWN || type == messages[i].type)) {
      // message was previously visible, so make it expire now.
      messages[i].texpiry = now - 1;
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

void
PopupMessage::AddMessage(const TCHAR* text, const TCHAR *data)
{
  ScopeLock protect(mutex);

  StatusMessage msg = status_messages.First();
  const StatusMessage *found = status_messages.Find(text);
  if (found != NULL)
    msg = *found;

  if (enable_sound && msg.sound != NULL)
    PlayResource(msg.sound);

  // TODO code: consider what is a sensible size?
  if (msg.visible) {
    TCHAR msgcache[1024];
    _tcscpy(msgcache, text);
    if (data != NULL) {
      _tcscat(msgcache, _T(" "));
      _tcscat(msgcache, data);
    }

    AddMessage(msg.delay_ms, MSG_USERINTERFACE, msgcache);
  }
}
