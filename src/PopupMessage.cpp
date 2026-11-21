// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PopupMessage.hpp"
#include "ui/window/SingleWindow.hpp"
#include "Screen/Layout.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Font.hpp"
#include "Look/DialogLook.hpp"
#include "Look/MapLook.hpp"
#include "Look/Colors.hpp"
#include "Renderer/MapScaleRenderer.hpp"
#include "Audio/Sound.hpp"
#include "StatusMessage.hpp"
#include "UISettings.hpp"
#include "util/CharUtil.hxx"
#include "util/StringAPI.hxx"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <memory>

#ifdef HAVE_VIBRATOR
/** the minimum time between two haptic feedbacks for new messages */
static constexpr std::chrono::steady_clock::duration HAPTIC_INTERVAL =
  std::chrono::seconds(2);
#endif

static constexpr auto PROGRESS_HOLD = std::chrono::seconds(1);

/** shortcut: English tokens, add an xcs level if custom files need more */
static bool
HasIgnoreCase(const char *text, const char *needle) noexcept
{
  const size_t n = StringLength(needle);
  for (const char *p = text; *p != '\0'; ++p)
    if (StringIsEqualIgnoreCase(p, needle, n))
      return true;
  return false;
}

static bool
HasToken(const char *text, const char *needle) noexcept
{
  const size_t n = StringLength(needle);
  for (const char *p = text; *p != '\0'; ++p) {
    if (p != text && IsAlphaASCII(p[-1]))
      continue;
    if (StringIsEqualIgnoreCase(p, needle, n) && !IsAlphaASCII(p[n]))
      return true;
  }
  return false;
}

static unsigned
ClassifyStatus(const char *text) noexcept
{
  if (HasToken(text, "stall") || HasToken(text, "overspeed") ||
      HasToken(text, "failed") || HasToken(text, "error"))
    return 2;
  if (HasToken(text, "warning") || HasToken(text, "battery") ||
      HasToken(text, "sunset") || HasToken(text, "invalid") ||
      HasIgnoreCase(text, "below final") ||
      HasIgnoreCase(text, "through terrain") ||
      HasIgnoreCase(text, "wind change") ||
      HasIgnoreCase(text, "waiting for gps") ||
      HasIgnoreCase(text, "no polar"))
    return 1;
  return 0;
}

static Color
StatusFill(unsigned rank) noexcept
{
  if (rank >= 2)
    return COLOR_AIRSPACE_WARNING_INSIDE;
  if (rank == 1)
    return COLOR_AIRSPACE_WARNING_NEAR;
  return COLOR_WHITE;
}

static const MaskedIcon *
StatusIcon(const MapLook &look, const char *text) noexcept
{
  if (HasToken(text, "simulator"))
    return &look.simulator_icon;
  if (HasToken(text, "repository"))
    return &look.database_icon;
  if (HasToken(text, "battery"))
    return &look.battery_icon;
  if (HasIgnoreCase(text, "waiting for gps"))
    return &look.waiting_for_fix_icon;
  if (HasToken(text, "gps"))
    return &look.no_gps_icon;
  if (HasIgnoreCase(text, "through terrain"))
    return &look.terrain_warning_icon;
  if (HasToken(text, "terrain"))
    return &look.waypoint.mountain_top_icon;
  if (HasToken(text, "stall") || HasToken(text, "overspeed") ||
      HasToken(text, "polar") || HasToken(text, "takeoff"))
    return &look.plane_icon;
  if (HasToken(text, "failed") || HasToken(text, "error"))
    return &look.abort_mode_icon;
  if (HasToken(text, "warning"))
    return &look.traffic_warning_icon;
  return nullptr;
}

static unsigned
HudPillMaxWidth(const PixelRect &hud) noexcept
{
  unsigned max_w = hud.GetWidth();
  const unsigned pad = Layout::GetTextPadding();
  if (max_w > 2 * pad)
    max_w -= 2 * pad;
  return max_w;
}

void
PopupMessage::Message::Set(Type _type,
                           std::chrono::steady_clock::duration _tshow,
                           const char *_text,
                           std::chrono::steady_clock::time_point now,
                           const char *_sound) noexcept
{
  type = _type;
  tshow = _tshow;
  tstart = now;
  texpiry = now;
  text = _text;
  sound = _sound;
  rank = ClassifyStatus(_text);
  fill = StatusFill(rank);
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
PopupMessage::Message::IsVisible(std::chrono::steady_clock::time_point now) const noexcept
{
  return !IsUnknown() && texpiry >= now;
}

PopupMessage::PopupMessage(UI::SingleWindow &_parent, const DialogLook &_look,
                           const MapLook &_map_look,
                           const UISettings &_settings) noexcept
  :parent(_parent), look(_look), map_look(_map_look),
   settings(_settings)
{
}

void
PopupMessage::Create(const PixelRect _rc) noexcept
{
  rc = _rc;

  WindowStyle style;
  style.Hide();

  PaintWindow::Create(parent, GetRect(), style);
  SetTransparent();
}

bool
PopupMessage::OnMouseDown(PixelPoint p) noexcept
{
  unsigned progress_h = 0;
  {
    const std::lock_guard lock{mutex};
    if (!progress_slots.empty()) {
      const Font &font = GetFont();
      const unsigned max_w = HudPillMaxWidth(rc);
      const unsigned gap = Layout::GetTextPadding();
      for (const auto &s : progress_slots) {
        if (progress_h > 0)
          progress_h += gap;
        progress_h += MapLabelSize(font, MapLabel{s.text.c_str()},
                                   max_w).height;
      }
    }
  }

  if (p.y < int(progress_h))
    return true;

  Acknowledge(MSG_UNKNOWN);
  return true;
}

const Font &
PopupMessage::GetFont() const noexcept
{
  return look.bold_font;
}

void
PopupMessage::OnPaint(Canvas &canvas) noexcept
{
  const Font &font = GetFont();
  const unsigned gap = Layout::GetTextPadding();

  struct ProgressView {
    StaticString<128> text;
    unsigned range;
    unsigned position;
  };
  StaticArray<ProgressView, MAX_PILLS> progress;
  StaticArray<StatusPill, MAX_PILLS> pills;
  {
    const std::lock_guard lock{mutex};
    for (const auto &s : progress_slots) {
      auto &v = progress.append();
      v.text = s.text;
      v.range = s.range;
      v.position = s.position;
    }
    pills = status_pills;
  }

  const unsigned max_w = HudPillMaxWidth(rc);
  const int win_w = int(canvas.GetWidth());
  int bottom = 0;

  auto draw = [&](const MapLabel &label) noexcept {
    const PixelSize size = MapLabelSize(font, label, max_w);
    bottom += int(size.height);
    int left = (win_w - int(size.width)) / 2;
    if (left < 0)
      left = 0;
    DrawMapLabel(canvas, font, left, bottom, label, max_w);
    bottom += int(gap);
  };

  for (const auto &p : progress) {
    const MapLabel progress_label{
      p.text.c_str(), StatusIcon(map_look, p.text.c_str()), COLOR_WHITE,
      nullptr, p.position, p.range,
    };
    draw(progress_label);
  }

  for (const auto &s : pills)
    draw(MapLabel{s.text.c_str(), s.icon, s.fill});
}

PixelRect
PopupMessage::GetRect() const noexcept
{
  const Font &font = GetFont();
  const unsigned gap = Layout::GetTextPadding();

  if (progress_slots.empty() && status_pills.empty())
    /* Window::IsDefined() is size.width > 0; stay hidden at 1x1 */
    return {rc.left, rc.top, rc.left + 1, rc.top + 1};

  const unsigned max_w = HudPillMaxWidth(rc);

  unsigned width = 0;
  unsigned height = 0;
  auto add = [&](const MapLabel &label) noexcept {
    const PixelSize size = MapLabelSize(font, label, max_w);
    width = std::max(width, size.width);
    if (height > 0)
      height += gap;
    height += size.height;
  };

  for (const auto &s : progress_slots) {
    const MapLabel label{
      s.text.c_str(), StatusIcon(map_look, s.text.c_str()), COLOR_WHITE,
      nullptr, s.position, s.range,
    };
    add(label);
  }

  for (const auto &s : status_pills)
    add(MapLabel{s.text.c_str(), s.icon, s.fill});

  const int left = rc.left + (int(rc.GetWidth()) - int(width)) / 2;
  const int top = rc.top + int(gap);

  return {left, top, left + int(width), top + int(height)};
}

void
PopupMessage::UpdateLayout(PixelRect _rc) noexcept
{
  rc = _rc;

  if (HasPills()) {
    Move(GetRect());
    BringToTop();
    Invalidate();
  }
}

void
PopupMessage::CollectStatusPills() noexcept
{
  struct Item {
    std::chrono::steady_clock::time_point tstart;
    unsigned rank;
    const char *text;
    Color fill;
  } items[MAXMESSAGES];
  unsigned n = 0;

  const auto now = std::chrono::steady_clock::now();
  for (unsigned i = 0; i < MAXMESSAGES; ++i) {
    if (!messages[i].IsVisible(now))
      continue;
    items[n++] = {messages[i].tstart, messages[i].rank,
                  messages[i].text.c_str(), messages[i].fill};
  }

  std::sort(items, items + n, [](const Item &a, const Item &b) {
    if (a.rank != b.rank)
      return a.rank > b.rank;
    return a.tstart > b.tstart;
  });

  status_pills.clear();
  const unsigned take = std::min(n, MAX_PILLS);
  for (unsigned i = 0; i < take; ++i) {
    auto &p = status_pills.append();
    p.text = items[i].text;
    p.fill = items[i].fill;
    p.icon = StatusIcon(map_look, items[i].text);
  }
}

void
PopupMessage::UpdateTextAndLayout() noexcept
{
  if (!HasPills()) {
    Hide();
  } else {
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

  bool changed = false;
  [[maybe_unused]] bool appeared = false;
  for (unsigned i = 0; i < MAXMESSAGES; ++i) {
    if (!messages[i].IsUnknown() && messages[i].IsNew())
      appeared = true;

    changed = messages[i].Update(now) || changed;
  }

  unsigned i = 0;
  while (i < progress_slots.size()) {
    const auto until = progress_slots[i].hold_until;
    if (until != std::chrono::steady_clock::time_point{} && now >= until) {
      if (i + 1 < progress_slots.size())
        progress_slots[i] = std::move(progress_slots.back());
      progress_slots.shrink(progress_slots.size() - 1);
      changed = true;
    } else {
      ++i;
    }
  }

  if (!changed)
    return false;

  CollectStatusPills();
  lock.unlock();

#ifdef HAVE_VIBRATOR
  /* a burst of messages shall not turn into a burst of vibrations */
  if (appeared && now >= last_haptic + HAPTIC_INTERVAL) {
    last_haptic = now;
    PlayHapticFeedback(HapticFeedbackType::NOTIFICATION);
  }
#endif

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
                         const char *Text, const char *snd) noexcept
{
  const auto now = std::chrono::steady_clock::now();

  int i = GetEmptySlot();
  messages[i].Set(type, tshow, Text, now, snd);
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
    if (enable_sound && messages[imax].sound != nullptr)
      PlayResource(messages[imax].sound);
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

int
PopupMessage::FindProgressLocked(const void *key) const noexcept
{
  for (unsigned i = 0; i < progress_slots.size(); ++i)
    if (progress_slots[i].key == key)
      return int(i);
  return -1;
}

bool
PopupMessage::HasPills() const noexcept
{
  return !progress_slots.empty() || !status_pills.empty();
}

void
PopupMessage::BeginProgress(const void *key, const char *text) noexcept
{
  {
    const std::lock_guard lock{mutex};
    const int i = FindProgressLocked(key);
    if (i >= 0) {
      auto &s = progress_slots[i];
      s.text = text != nullptr ? text : "";
      s.hold_until = {};
    } else if (!progress_slots.full()) {
      auto &s = progress_slots.append();
      s.key = key;
      s.text = text != nullptr ? text : "";
      s.range = 0;
      s.position = 0;
      s.env = std::make_unique<SlotEnv>(*this, key);
    }
  }

  UpdateTextAndLayout();
}

void
PopupMessage::ClearProgress(const void *key) noexcept
{
  {
    const std::lock_guard lock{mutex};
    const int i = FindProgressLocked(key);
    if (i < 0)
      return;
    auto &s = progress_slots[i];
    if (s.range > 0)
      s.position = s.range;
    s.hold_until = std::chrono::steady_clock::now() + PROGRESS_HOLD;
  }

  if (IsDefined())
    Invalidate();
}

OperationEnvironment &
PopupMessage::GetProgressEnvironment(const void *key) noexcept
{
  const std::lock_guard lock{mutex};
  const int i = FindProgressLocked(key);
  assert(i >= 0);
  return *progress_slots[i].env;
}

void
PopupMessage::SetProgressText(const void *key, const char *text) noexcept
{
  {
    const std::lock_guard lock{mutex};
    const int i = FindProgressLocked(key);
    if (i < 0)
      return;
    progress_slots[i].text = text != nullptr ? text : "";
  }

  UpdateTextAndLayout();
}

void
PopupMessage::SetProgressRange(const void *key, unsigned range) noexcept
{
  {
    const std::lock_guard lock{mutex};
    const int i = FindProgressLocked(key);
    if (i < 0)
      return;
    if (progress_slots[i].range == range)
      return;
    progress_slots[i].range = range;
  }

  if (IsDefined())
    Invalidate();
}

void
PopupMessage::SetProgressPosition(const void *key, unsigned position) noexcept
{
  {
    const std::lock_guard lock{mutex};
    const int i = FindProgressLocked(key);
    if (i < 0)
      return;
    if (progress_slots[i].position == position)
      return;
    progress_slots[i].position = position;
  }

  if (IsDefined())
    Invalidate();
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

    AddMessage(msg.delay, MSG_USERINTERFACE, msgcache, msg.sound);
  }
}
