// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PopupMessage.hpp"
#include "ui/window/SingleWindow.hpp"
#include "Screen/Layout.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Look/DialogLook.hpp"
#include "Audio/Sound.hpp"
#include "StatusMessage.hpp"
#include "UISettings.hpp"

#include <algorithm>

using std::min;
using std::max;

void
PopupMessage::Message::Set(Type _type,
                           std::chrono::steady_clock::duration _tshow,
                           const char *_text,
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
    buffer.append("\r\n");
  buffer.append(text);
  return true;
}

PopupMessage::PopupMessage(UI::SingleWindow &_parent, const DialogLook &_look,
                           const UISettings &_settings) noexcept
  :parent(_parent), look(_look),
   settings(_settings)
{
  renderer.SetCenter();
  text.clear();
}

void
PopupMessage::Create(const PixelRect _rc) noexcept
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
PopupMessage::OnMouseDown([[maybe_unused]] PixelPoint p) noexcept
{
  // acknowledge with click/touch
  Acknowledge(MSG_UNKNOWN);

  return true;
}

void
PopupMessage::OnPaint(Canvas &canvas) noexcept
{
  canvas.Clear(look.dark_mode ? look.background_color : COLOR_WHITE);

  auto rc = GetClientRect();
#ifndef USE_WINUSER
  canvas.DrawOutlineRectangle(rc,
                              look.dark_mode ? COLOR_GRAY : COLOR_BLACK);
#endif

  const int padding = Layout::GetTextPadding();
  rc.Grow(-padding);

  canvas.SetTextColor(look.text_color);
  canvas.SetBackgroundTransparent();
  canvas.Select(look.text_font);

  renderer.Draw(canvas, rc, text);
}

inline unsigned
PopupMessage::CalculateWidth() const noexcept
{
  if (settings.popup_message_position == UISettings::PopupMessagePosition::TOP_LEFT)
    return rc.GetWidth();
  else
    return unsigned(rc.GetWidth() * 0.9);
}

PixelRect
PopupMessage::GetRect(PixelSize size) const noexcept
{
  if (settings.popup_message_position == UISettings::PopupMessagePosition::TOP_LEFT) {
    return PixelRect{rc.left, rc.top,
                     static_cast<int>(rc.left) + static_cast<int>(size.width),
                     static_cast<int>(rc.top) + static_cast<int>(size.height)};
  } else {
    return PixelRect::Centered(rc.GetCenter(), size);
  }
}

PixelRect
PopupMessage::GetRect() const noexcept
{
  const unsigned width = CalculateWidth();
  const unsigned height = renderer.GetHeight(look.text_font, width, text)
    + 2 * Layout::GetTextPadding();

  return GetRect({width, height});
}

void
PopupMessage::UpdateLayout(PixelRect _rc) noexcept
{
  rc = _rc;

  if (!text.empty()) {
    renderer.InvalidateLayout();
    Move(GetRect());
    Invalidate();
  }
}

void
PopupMessage::UpdateTextAndLayout() noexcept
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
PopupMessage::Render() noexcept
{
  if (parent.HasDialog())
    return false;

  std::unique_lock lock{mutex};

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
    return false;
  }

  // ok, we've changed the visible messages, so need to regenerate the
  // text box

  text.clear();
  n_visible = 0;
  for (unsigned i = 0; i < MAXMESSAGES; ++i)
    if (messages[i].AppendTo(text, now))
      n_visible++;

  lock.unlock();

  UpdateTextAndLayout();

  return true;
}

int
PopupMessage::GetEmptySlot() noexcept
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
                         const char *Text) noexcept
{
  const auto now = std::chrono::steady_clock::now();

  int i = GetEmptySlot();
  messages[i].Set(type, tshow, Text, now);
}

void
PopupMessage::Repeat(Type type) noexcept
{
  int imax = -1;

  const std::lock_guard lock{mutex};

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
}

bool
PopupMessage::Acknowledge(Type type) noexcept
{
  const std::lock_guard lock{mutex};
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
//  - Text to display (including multiple languages)
//  - Text to display extra - NOT multiple language
//    (eg: If Airspace Warning - what details - airfield name is in data file,
//    already covers multiple languages).
//  - ShowStatusMessage - including font size and delay
//  - Sound to play - What sound to play
//  - Log - Keep the message on the log/history window (goes to log file and
//  history)
//
// TODO code: (need to discuss) Consider moving almost all this functionality
// into AddMessage ?

void
PopupMessage::AddMessage(const char* text, const char *data) noexcept
{
  const std::lock_guard lock{mutex};

  const auto &msg = FindStatusMessage(text);

  if (enable_sound && msg.sound != nullptr)
    PlayResource(msg.sound);

  // TODO code: consider what is a sensible size?
  if (msg.visible) {
    char msgcache[1024];
    strcpy(msgcache, text);
    if (data != nullptr) {
      strcat(msgcache, " ");
      strcat(msgcache, data);
    }

    AddMessage(msg.delay, MSG_USERINTERFACE, msgcache);
  }
}
