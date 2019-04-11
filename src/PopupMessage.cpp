/*

Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Screen/SingleWindow.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Canvas.hpp"
#include "Look/DialogLook.hpp"
#include "Audio/Sound.hpp"
#include "StatusMessage.hpp"
#include "UISettings.hpp"

#include <tchar.h>
#include <algorithm>

using std::min;
using std::max;

void
PopupMessage::Message::Set(Type _type,
                           std::chrono::steady_clock::duration _tshow,
                           const TCHAR *_text,
                           std::chrono::steady_clock::time_point now) noexcept
{
  type = _type;
  tshow = _tshow;
  tstart = now;
  texpiry = now;
  text = _text;
}

bool
PopupMessage::Message::Update(std::chrono::steady_clock::time_point now) noexcept
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
PopupMessage::Message::AppendTo(StaticString<2000> &buffer,
                                std::chrono::steady_clock::time_point now) noexcept
{
  if (IsUnknown())
    // ignore unknown messages
    return false;

  if (texpiry < now) {
    texpiry = tstart - std::chrono::steady_clock::duration(1);
    // reset expiry so we don't refresh
    return false;
  }

  if (!buffer.empty())
    buffer.append(_T("\r\n"));
  buffer.append(text);
  return true;
}

PopupMessage::PopupMessage(SingleWindow &_parent, const DialogLook &_look,
                           const UISettings &_settings)
  :parent(_parent), look(_look),
   settings(_settings),
   n_visible(0),
   enable_sound(true)
{
  renderer.SetCenter();
  text.clear();
}

void
PopupMessage::Create(const PixelRect _rc)
{
  rc = _rc;

  WindowStyle style;
#ifdef USE_WINUSER
  style.Border();
#endif
  style.Hide();

  PaintWindow::Create(parent, GetRect(), style);
}

bool
PopupMessage::OnMouseDown(PixelPoint p)
{
  // acknowledge with click/touch
  Acknowledge(MSG_UNKNOWN);

  return true;
}

void
PopupMessage::OnPaint(Canvas &canvas)
{
  canvas.ClearWhite();

  auto rc = GetClientRect();
#ifndef USE_WINUSER
  canvas.DrawOutlineRectangle(rc.left, rc.top, rc.right, rc.bottom,
                              COLOR_BLACK);
#endif

  const int padding = Layout::GetTextPadding();
  rc.Grow(-padding);

  canvas.SetTextColor(look.text_color);
  canvas.SetBackgroundTransparent();
  canvas.Select(look.text_font);

  renderer.Draw(canvas, rc, text);
}

inline unsigned
PopupMessage::CalculateWidth() const
{
  if (settings.popup_message_position == UISettings::PopupMessagePosition::TOP_LEFT)
    // TODO code: this shouldn't be hard-coded
    return Layout::FastScale(206);
  else
    return unsigned(rc.GetWidth() * 0.9);
}

PixelRect
PopupMessage::GetRect(unsigned width, unsigned height) const
{
  PixelRect rthis;

  if (settings.popup_message_position == UISettings::PopupMessagePosition::TOP_LEFT) {
    rthis.top = 0;
    rthis.left = 0;
    rthis.bottom = height;
    rthis.right = width;
  } else {
    const int midx = (rc.right + rc.left) / 2;
    const int midy = (rc.bottom + rc.top) / 2;
    const int h1 = height / 2;
    const int h2 = height - h1;
    rthis.left = midx-width/2;
    rthis.right = midx+width/2;
    rthis.top = midy-h1;
    rthis.bottom = midy+h2;
  }

  return rthis;
}

PixelRect
PopupMessage::GetRect() const
{
  const unsigned width = CalculateWidth();
  const unsigned height = renderer.GetHeight(look.text_font, width, text)
    + 2 * Layout::GetTextPadding();

  return GetRect(width, height);
}

void
PopupMessage::UpdateLayout(PixelRect _rc)
{
  rc = _rc;

  if (!text.empty()) {
    renderer.InvalidateLayout();
    Move(GetRect());
    Invalidate();
  }
}

void
PopupMessage::UpdateTextAndLayout()
{
  if (text.empty()) {
    Hide();
  } else {
    renderer.InvalidateLayout();

    Move(GetRect());
    ShowOnTop();
    Invalidate();
  }
}

bool
PopupMessage::Render()
{
  if (parent.HasDialog())
    return false;

  mutex.Lock();

  const auto now = std::chrono::steady_clock::now();

  // this has to be done quickly, since it happens in GUI thread
  // at subsecond interval

  // first loop through all messages, and determine which should be
  // made invisible that were previously visible, or
  // new messages

  bool changed = false;
  for (unsigned i = 0; i < MAXMESSAGES; ++i)
    changed = messages[i].Update(now) || changed;

  if (!changed) {
    mutex.Unlock();
    return false;
  }

  // ok, we've changed the visible messages, so need to regenerate the
  // text box

  text.clear();
  n_visible = 0;
  for (unsigned i = 0; i < MAXMESSAGES; ++i)
    if (messages[i].AppendTo(text, now))
      n_visible++;

  mutex.Unlock();

  UpdateTextAndLayout();

  return true;
}

int
PopupMessage::GetEmptySlot()
{
  // find oldest message that is no longer visible

  // todo: make this more robust with respect to message types and if can't
  // find anything to remove..
  unsigned imin = 0;
  std::chrono::steady_clock::time_point tmin{};
  for (unsigned i = 0; i < MAXMESSAGES; i++) {
    if (i == 0 || messages[i].tstart < tmin) {
      tmin = messages[i].tstart;
      imin = i;
    }
  }
  return imin;
}

void
PopupMessage::AddMessage(std::chrono::steady_clock::duration tshow, Type type,
                         const TCHAR *Text) noexcept
{
  assert(mutex.IsLockedByCurrent());

  const auto now = std::chrono::steady_clock::now();

  int i = GetEmptySlot();
  messages[i].Set(type, tshow, Text, now);
}

void
PopupMessage::Repeat(Type type)
{
  int imax = -1;

  mutex.Lock();
  const auto now = std::chrono::steady_clock::now();

  // find most recent non-visible message

  std::chrono::steady_clock::time_point tmax{};
  for (unsigned i = 0; i < MAXMESSAGES; i++) {
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
  const auto now = std::chrono::steady_clock::now();

  for (unsigned i = 0; i < MAXMESSAGES; i++) {
    if (messages[i].texpiry > messages[i].tstart &&
        (type == MSG_UNKNOWN || type == messages[i].type)) {
      // message was previously visible, so make it expire now.
      messages[i].texpiry = now - std::chrono::steady_clock::duration(1);
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

  const auto &msg = FindStatusMessage(text);

  if (enable_sound && msg.sound != nullptr)
    PlayResource(msg.sound);

  // TODO code: consider what is a sensible size?
  if (msg.visible) {
    TCHAR msgcache[1024];
    _tcscpy(msgcache, text);
    if (data != nullptr) {
      _tcscat(msgcache, _T(" "));
      _tcscat(msgcache, data);
    }

    AddMessage(msg.delay, MSG_USERINTERFACE, msgcache);
  }
}
