// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InfoBoxArrangeWindow.hpp"
#include "InfoBoxLayout.hpp"
#include "InfoBoxManager.hpp"
#include "Content/Factory.hpp"
#include "Asset.hpp"
#include "Dialogs/HelpDialog.hpp"
#include "Hardware/CPU.hpp"
#include "Language/Language.hpp"
#include "Look/DialogLook.hpp"
#include "Look/InfoBoxLook.hpp"
#include "Screen/Layout.hpp"
#include "UIGlobals.hpp"
#include "ui/canvas/Brush.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/event/KeyCode.hpp"
#include "ui/window/SingleWindow.hpp"
#include "util/StaticString.hxx"
#include "util/StringCompare.hxx"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scope.hpp"
#endif

#ifdef ENABLE_SDL
#include <SDL_keyboard.h>
#endif

#include <algorithm>
#include <cassert>
#include <cstdlib>

namespace {

/**
 * How much of the map shines through the overlay?  Only a hint of it
 * is left, so that the text on the backdrop stays legible.
 */
constexpr uint8_t OVERLAY_ALPHA = 0xe6;

/** how long a displaced InfoBox takes to slide into its new slot */
constexpr auto SHUFFLE_DURATION = std::chrono::milliseconds(250);

/** how long an InfoBox has to be held before the picker opens */
constexpr auto PICKER_DELAY = std::chrono::milliseconds(600);

/**
 * How much the card grows when it is picked up, relative to the slot
 * it came from.
 */
[[gnu::pure]]
int
GetDragLift() noexcept
{
  return Layout::Scale(5);
}

/**
 * The distance the finger has to travel before the InfoBox detaches
 * from its slot; below that, the press is still a tap.
 */
[[gnu::pure]]
int
GetDeadZone() noexcept
{
  return Layout::Scale(8);
}

/**
 * Are the InfoBoxes arranged in columns?  This does not follow the
 * screen orientation: several geometries put the InfoBoxes into
 * columns on a portrait screen, and into rows on a landscape one.
 */
[[gnu::pure]]
bool
HasColumns(const InfoBoxLayout::Layout &layout) noexcept
{
  unsigned rows = 0, columns = 0;

  for (unsigned i = 0; i < layout.count; ++i) {
    const PixelPoint p = layout.positions[i].GetCenter();
    bool new_row = true, new_column = true;

    for (unsigned j = 0; j < i; ++j) {
      const PixelPoint q = layout.positions[j].GetCenter();
      new_row &= q.y != p.y;
      new_column &= q.x != p.x;
    }

    rows += new_row;
    columns += new_column;
  }

  /* more rows than columns means the InfoBoxes stand side by side */
  return rows > columns;
}

/**
 * Is a shift key held down?  The window layer passes only the key
 * code, so Shift+Tab has to be recognised here; this follows
 * IsCtrlKeyPressed() in GlueMapWindowEvents.
 */
[[gnu::pure]]
bool
IsShiftKeyPressed() noexcept
{
#ifdef ENABLE_SDL
  return SDL_GetModState() & (KMOD_LSHIFT | KMOD_RSHIFT);
#elif defined(USE_WINUSER)
  return GetKeyState(VK_SHIFT) & 0x8000;
#else
  /* X11 sends XK_ISO_Left_Tab instead; elsewhere Tab only moves
     forwards */
  return false;
#endif
}

} // namespace

bool InfoBoxArrangeWindow::card_floating;

InfoBoxArrangeWindow::InfoBoxArrangeWindow(const InfoBoxLook &_look,
                                           const DialogLook &_dialog_look,
                                           Style _style) noexcept
  :look(_look), dialog_look(_dialog_look), style(_style),
   button_renderer(_dialog_look.button)
{
  ResetCardNumbers();
}

unsigned
InfoBoxArrangeWindow::AddButton(unsigned row, const char *caption,
                                Callback callback) noexcept
{
  auto &button = buttons.append();
  button.caption = caption;
  button.callback = std::move(callback);
  button.row = row;
  button.enabled = true;
  return buttons.size() - 1;
}

void
InfoBoxArrangeWindow::SetButtonEnabled(unsigned i, bool enabled) noexcept
{
  buttons[i].enabled = enabled;
  Invalidate();
}

void
InfoBoxArrangeWindow::Create(ContainerWindow &parent,
                             const PixelRect &rc) noexcept
{
  WindowStyle window_style;
  window_style.Hide();
  window_style.TabStop();
  PaintWindow::Create(parent, rc, window_style);

#ifndef USE_WINUSER
  if (style == Style::MAP)
    /* the map below must still be painted (and invalidated), even
       though this window covers the whole screen */
    SetTransparent();
#endif
}

void
InfoBoxArrangeWindow::SetPanel(InfoBoxSettings::Panel &_panel) noexcept
{
  panel = &_panel;
  ResetCardNumbers();
}

void
InfoBoxArrangeWindow::SetLayout(const InfoBoxLayout::Layout &_layout,
                                PixelRect _content) noexcept
{
  layout = &_layout;
  content = _content;
  columns = HasColumns(_layout);
}

/*
 * geometry
 */

PixelPoint
InfoBoxArrangeWindow::SlotCenter(unsigned slot) const noexcept
{
  return layout->positions[slot].GetCenter();
}

int
InfoBoxArrangeWindow::Along(PixelPoint p) const noexcept
{
  return columns ? p.x : p.y;
}

int
InfoBoxArrangeWindow::Across(PixelPoint p) const noexcept
{
  return columns ? p.y : p.x;
}

int
InfoBoxArrangeWindow::FindSlot(PixelPoint p) const noexcept
{
  for (unsigned i = 0; i < layout->count; ++i)
    if (layout->positions[i].Contains(p))
      return i;

  return -1;
}

PixelRect
InfoBoxArrangeWindow::GetFloatingRect() const noexcept
{
  PixelRect rc = drag->start_rect;
  rc.Offset(drag->pointer.x - drag->origin.x,
            drag->pointer.y - drag->origin.y);

  /* the padding stays behind in the slot; the card itself is picked
     up and grows a little */
  rc.Grow(GetDragLift() - (int)look.preview_padding);
  return rc;
}

int
InfoBoxArrangeWindow::GetButtonGap() noexcept
{
  return Layout::Scale(8);
}

unsigned
InfoBoxArrangeWindow::GetButtonRowCount() const noexcept
{
  unsigned count = 1;
  for (const auto &i : buttons)
    count = std::max(count, i.row + 1);

  return count;
}

unsigned
InfoBoxArrangeWindow::GetRowButtonCount(unsigned row) const noexcept
{
  unsigned count = 0;
  for (const auto &i : buttons)
    if (i.row == row)
      ++count;

  return count;
}

unsigned
InfoBoxArrangeWindow::GetIndexInRow(unsigned i) const noexcept
{
  unsigned index = 0;
  for (unsigned j = 0; j < i; ++j)
    if (buttons[j].row == buttons[i].row)
      ++index;

  return index;
}

int
InfoBoxArrangeWindow::GetButtonWidth(unsigned row) const noexcept
{
  const int count = GetRowButtonCount(row);
  assert(count > 0);

  const int gap = GetButtonGap();
  const int width = ((int)content.GetWidth() - (count + 1) * gap) / count;

  return style == Style::MAP
    /* on the map the buttons stay compact; they are not what the mode
       is about */
    ? std::min(width, Layout::Scale(80))
    /* in a dialog they fill the width, like the description above
       them */
    : width;
}

int
InfoBoxArrangeWindow::GetButtonHeight() const noexcept
{
  const int gap = GetButtonGap();
  const int rows = GetButtonRowCount();

  /* the rows share the lower half of the area between the cards, and
     only get flatter than a dialog control when even that is too
     little */
  const int available = ((int)content.GetHeight() / 2 - gap) / rows - gap;

  return std::clamp(available,
                    (int)Layout::GetMinimumControlHeight(),
                    (int)Layout::GetMaximumControlHeight());
}

PixelRect
InfoBoxArrangeWindow::GetButtonRowRect(unsigned row) const noexcept
{
  const int count = GetRowButtonCount(row);
  const int height = GetButtonHeight();
  const int gap = GetButtonGap();
  const int total = count * GetButtonWidth(row) + (count - 1) * gap;

  PixelRect r;
  r.bottom = content.bottom - gap - (int)row * (height + gap);
  r.top = r.bottom - height;
  r.left = (content.left + content.right - total) / 2;
  r.right = r.left + total;
  return r;
}

PixelRect
InfoBoxArrangeWindow::GetButtonBlockRect() const noexcept
{
  PixelRect r = GetButtonRowRect(0);

  for (unsigned row = 1; row < GetButtonRowCount(); ++row) {
    const PixelRect above = GetButtonRowRect(row);
    r.top = above.top;
    r.left = std::min(r.left, above.left);
    r.right = std::max(r.right, above.right);
  }

  return r;
}

PixelRect
InfoBoxArrangeWindow::GetButtonRect(int i) const noexcept
{
  const unsigned row = buttons[i].row;
  const int width = GetButtonWidth(row);

  PixelRect r = GetButtonRowRect(row);
  r.left += (int)GetIndexInRow(i) * (width + GetButtonGap());
  r.right = r.left + width;
  return r;
}

double
InfoBoxArrangeWindow::GetSlotFraction(unsigned slot) const noexcept
{
  const PixelPoint center = SlotCenter(slot);

  unsigned count = 0, index = 0;

  for (unsigned i = 0; i < layout->count; ++i) {
    const PixelPoint p = SlotCenter(i);
    if (Along(p) != Along(center))
      /* another row or column */
      continue;

    if (Across(p) < Across(center))
      ++index;

    ++count;
  }

  return (index + 0.5) / count;
}

double
InfoBoxArrangeWindow::GetButtonFraction(unsigned i) const noexcept
{
  return (GetIndexInRow(i) + 0.5) / GetRowButtonCount(buttons[i].row);
}

double
InfoBoxArrangeWindow::GetButtonCrossFraction(unsigned i) const noexcept
{
  if (!columns)
    return GetButtonFraction(i);

  /* the rows of the block are stacked across the axis, and row 0 is
     the bottom one */
  const unsigned rows = GetButtonRowCount();
  return (rows - 1 - buttons[i].row + 0.5) / rows;
}

std::optional<int>
InfoBoxArrangeWindow::FindSlotGroup(int along, int direction) const noexcept
{
  std::optional<int> group;

  for (unsigned i = 0; i < layout->count; ++i) {
    const int g = Along(SlotCenter(i));
    if ((g - along) * direction <= 0)
      continue;

    if (!group || (g - *group) * direction < 0)
      group = g;
  }

  return group;
}

int
InfoBoxArrangeWindow::FindSlotAt(int group, double fraction) const noexcept
{
  int best = -1;
  double best_distance = 0;

  for (unsigned i = 0; i < layout->count; ++i) {
    if (Along(SlotCenter(i)) != group)
      continue;

    const double distance = std::abs(GetSlotFraction(i) - fraction);

    if (best < 0 || distance < best_distance) {
      best = i;
      best_distance = distance;
    }
  }

  return best;
}

int
InfoBoxArrangeWindow::FindButtonAt(unsigned row,
                                   double fraction) const noexcept
{
  int best = -1;
  double best_distance = 0;

  for (unsigned i = 0; i < buttons.size(); ++i) {
    if (buttons[i].row != row || !buttons[i].enabled)
      continue;

    const double distance = std::abs(GetButtonFraction(i) - fraction);

    if (best < 0 || distance < best_distance) {
      best = i;
      best_distance = distance;
    }
  }

  return best;
}

int
InfoBoxArrangeWindow::FindNextInRow(int i, int dx) const noexcept
{
  /* the buttons of a row are added from left to right */
  for (int j = i + dx; j >= 0 && j < (int)buttons.size(); j += dx)
    if (buttons[j].row == buttons[i].row && buttons[j].enabled)
      return j;

  return -1;
}

int
InfoBoxArrangeWindow::FindButton(PixelPoint p) const noexcept
{
  for (unsigned i = 0; i < buttons.size(); ++i)
    if (GetButtonRect(i).Contains(p))
      return i;

  return -1;
}

PixelRect
InfoBoxArrangeWindow::GetPanelNameRect() const noexcept
{
  PixelRect rc = ToLocal(GetButtonBlockRect());
  rc.bottom = rc.top - Layout::Scale(12);
  rc.top = rc.bottom - (style == Style::MAP
                        ? (int)look.title_font_bold.GetHeight()
                        : 0);
  return rc;
}

PixelRect
InfoBoxArrangeWindow::ToLocal(PixelRect rc) const noexcept
{
  const auto origin = GetPosition().GetTopLeft();
  rc.Offset(-origin.x, -origin.y);
  return rc;
}

/*
 * painting
 */

void
InfoBoxArrangeWindow::DrawCard(Canvas &canvas, const PixelRect &rc,
                               unsigned slot, unsigned number,
                               CardState state) noexcept
{
  const int radius = look.preview_radius;
  const bool active = state == CardState::ACTIVE;

  canvas.SelectNullPen();
  canvas.Select(Brush{state == CardState::NORMAL
                      ? look.background_color
                      : look.preview_active_color});
  canvas.DrawRoundRectangle(rc, {radius * 2, radius * 2});

  if (state == CardState::FOCUSED) {
    /* leave only a ring of the highlight colour */
    const int width = look.preview_focus_width;
    PixelRect inner = rc;
    inner.Grow(-width);

    canvas.Select(Brush{look.background_color});
    canvas.DrawRoundRectangle(inner, {(radius - width) * 2,
                                      (radius - width) * 2});
  }

  /* the caption comes from the configuration and not from the InfoBox
     itself, because content providers overwrite the title at runtime
     (the Next Waypoint InfoBox shows the waypoint name, for
     example) */
  const char *caption =
    gettext(InfoBoxFactory::GetCaption(panel->contents[slot]));

  StaticString<8> number_text;
  number_text.Format("#%u", number);

  canvas.SetBackgroundTransparent();
  canvas.SetTextColor(active ? COLOR_WHITE : look.title.fg_color);

  canvas.Select(look.preview_number_font);
  const PixelSize number_size = canvas.CalcTextSize(number_text);

  canvas.Select(look.title_font_bold);
  const PixelSize caption_size = canvas.CalcTextSize(caption);

  /* both lines are clipped to the card, so that a caption which is
     too long for it does not run out into the backdrop; if they do
     not fit at all, they start at the top edge */
  int y = rc.top + std::max(0, ((int)rc.GetHeight()
                                - (int)(number_size.height
                                        + caption_size.height)) / 2);

  canvas.Select(look.preview_number_font);
  canvas.DrawClippedText({std::max(rc.left,
                                   rc.CenteredTopLeft(number_size).x), y},
                         rc, number_text);

  y += number_size.height;

  canvas.Select(look.title_font_bold);
  canvas.DrawClippedText({std::max(rc.left,
                                   rc.CenteredTopLeft(caption_size).x), y},
                         rc, caption);
}

InfoBoxArrangeWindow::CardState
InfoBoxArrangeWindow::GetCardState(unsigned slot) const noexcept
{
  if ((int)slot != described_slot || focused_button >= 0)
    return CardState::NORMAL;

  return selection == Selection::FOCUSED
    ? CardState::FOCUSED
    : CardState::ACTIVE;
}

void
InfoBoxArrangeWindow::PaintCards(Canvas &canvas) noexcept
{
  for (unsigned i = 0; i < layout->count; ++i) {
    if (drag && drag->following && i == drag->slot)
      /* this one follows the finger; its slot stays empty */
      continue;

    PixelRect rc = layout->positions[i];
    const PixelPoint offset = GetShuffleOffset(i);
    rc.Offset(offset.x, offset.y);
    rc.Grow(-(int)look.preview_padding);
    DrawCard(canvas, ToLocal(rc), i, card_number[i], GetCardState(i));
  }

  if (drag && drag->following)
    DrawCard(canvas, ToLocal(GetFloatingRect()), drag->slot,
             card_number[drag->slot], CardState::ACTIVE);
}

void
InfoBoxArrangeWindow::PaintButtons(Canvas &canvas) noexcept
{
  for (unsigned i = 0; i < buttons.size(); ++i) {
    button_renderer.SetCaption(buttons[i].caption);
    button_renderer.DrawButton(canvas, ToLocal(GetButtonRect(i)),
                               GetButtonState(i));
  }
}

void
InfoBoxArrangeWindow::PaintPanelName(Canvas &canvas) noexcept
{
  if (style != Style::MAP)
    /* the dialog names the panel itself */
    return;

  const char *name = gettext(panel->name);
  if (StringIsEmpty(name))
    return;

  canvas.SetBackgroundTransparent();
  canvas.SetTextColor(look.background_color);
  canvas.Select(look.title_font_bold);

  const PixelRect rc = GetPanelNameRect();
  canvas.TextAutoClipped(rc.CenteredTopLeft(canvas.CalcTextSize(name)), name);
}

void
InfoBoxArrangeWindow::PaintDescription(Canvas &canvas) noexcept
{
  if (described_slot < 0 || focused_button >= 0)
    /* nothing describes a button */
    return;

  const auto type = panel->contents[described_slot];
  const char *name = gettext(InfoBoxFactory::GetName(type));
  const char *description = InfoBoxFactory::GetDescription(type);

  PixelRect rc = ToLocal(content);
  rc.Grow(-Layout::Scale(8));

  /* keep clear of the panel name and the buttons; on a small screen
     the description is cut off instead of covering them */
  rc.bottom = std::min(rc.bottom, GetPanelNameRect().top - Layout::Scale(8));
  if (rc.bottom <= rc.top)
    return;

  canvas.SetBackgroundTransparent();
  canvas.SetTextColor(style == Style::MAP
                      ? look.background_color
                      : dialog_look.text_color);

  canvas.Select(dialog_look.bold_font);
  name_renderer.Draw(canvas, rc, name);
  rc.top += name_renderer.GetHeight(canvas, rc.GetWidth(), name)
    + Layout::Scale(4);

  if (description != nullptr && rc.top < rc.bottom) {
    const char *text = gettext(description);

    /* the small font is only used when the description does not fit
       into what is left */
    canvas.Select(dialog_look.text_font);
    if (description_renderer.GetHeight(canvas, rc.GetWidth(), text) >
        (unsigned)rc.GetHeight())
      canvas.Select(dialog_look.small_font);

    description_renderer.Draw(canvas, rc, text);
  }
}

void
InfoBoxArrangeWindow::OnPaint(Canvas &canvas) noexcept
{
  if (style == Style::MAP) {
#ifdef ENABLE_OPENGL
    const ScopeAlphaBlend alpha_blend;
    canvas.DrawFilledRectangle(canvas.GetRect(),
                               look.preview_backdrop_color
                               .WithAlpha(OVERLAY_ALPHA));
#else
    /* without alpha blending, hide the map instead of fading it */
    canvas.DrawFilledRectangle(canvas.GetRect(),
                               look.preview_backdrop_color);
#endif
  } else
    canvas.Clear(dialog_look.background_color);

  PaintDescription(canvas);
  PaintPanelName(canvas);
  PaintButtons(canvas);

  /* the cards come last: the one which follows the finger must not
     disappear behind anything */
  PaintCards(canvas);
}

/*
 * the InfoBoxes
 */

void
InfoBoxArrangeWindow::Exchange(unsigned a, unsigned b) noexcept
{
  std::swap(panel->contents[a], panel->contents[b]);
  std::swap(card_number[a], card_number[b]);
  OnArrangeModified();
}

void
InfoBoxArrangeWindow::ResetCardNumbers() noexcept
{
  for (unsigned i = 0; i < InfoBoxSettings::Panel::MAX_CONTENTS; ++i)
    card_number[i] = i + 1;
}

void
InfoBoxArrangeWindow::StartShuffle(unsigned slot,
                                   const PixelRect &from) noexcept
{
  if (HasEPaper() || IsSlowCPU())
    /* e-paper and slow CPUs snap instead of animating, the same gate
       the list and the map pan animations use */
    return;

  const PixelRect &to = layout->positions[slot];
  shuffle[slot].offset = {from.left - to.left, from.top - to.top};
  shuffle[slot].start = std::chrono::steady_clock::now();

  shuffle_timer.Schedule(std::chrono::milliseconds(16));
}

PixelPoint
InfoBoxArrangeWindow::GetShuffleOffset(unsigned slot) const noexcept
{
  const auto elapsed = std::chrono::steady_clock::now() - shuffle[slot].start;
  if (elapsed >= SHUFFLE_DURATION)
    return {0, 0};

  const double t = std::chrono::duration<double>(elapsed)
    / std::chrono::duration<double>(SHUFFLE_DURATION);

  /* ease in out cubic, the curve UIKit animates with by default */
  const double e = t < 0.5
    ? 4 * t * t * t
    : 1 - 4 * (1 - t) * (1 - t) * (1 - t);

  return {int(shuffle[slot].offset.x * (1 - e)),
          int(shuffle[slot].offset.y * (1 - e))};
}

void
InfoBoxArrangeWindow::OnShuffleTimer() noexcept
{
  Invalidate();

  for (unsigned i = 0; i < layout->count; ++i) {
    const auto offset = GetShuffleOffset(i);
    if (offset.x != 0 || offset.y != 0)
      return;
  }

  shuffle_timer.Cancel();
}

void
InfoBoxArrangeWindow::OnPickerTimer() noexcept
{
  if (!drag || drag->following)
    return;

  const unsigned slot = drag->slot;
  drag.reset();
  ReleaseCapture();
  Invalidate();

  ShowPicker(slot);
}

void
InfoBoxArrangeWindow::ShowPicker(unsigned slot) noexcept
{
  OnArrangeSuspend();

  if (InfoBoxManager::ShowInfoBoxPicker(*panel, slot)) {
    Invalidate();
    OnArrangeModified();
  }

  OnArrangeActivity();
  SetFocus();
}

void
InfoBoxArrangeWindow::ShowHelp() noexcept
{
  StaticString<768> text;
  text.clear();

  if (HasPointer())
    text = _("Drag an InfoBox onto another one to exchange the two.  "
             "Long press an InfoBox to choose a different InfoBox for "
             "that position.");

  if (HasCursorKeys()) {
    if (!text.empty())
      text.append("\n\n");

    text.append(_("Press Enter to take the selected InfoBox, move it with "
                  "the cursor keys and press Enter again to put it down.  "
                  "Pressing Enter twice without moving an InfoBox chooses "
                  "a different InfoBox for that position."));
  }

  if (extra_help != nullptr) {
    text.append("\n\n");
    text.append(extra_help);
  }

  /* the timeout must not end the mode behind the dialog */
  OnArrangeSuspend();
  HelpDialog(_("Arrange InfoBoxes"), text);
  OnArrangeActivity();
  SetFocus();
}

/*
 * the cursor keys
 */

int
InfoBoxArrangeWindow::FindNeighbour(PixelPoint origin,
                                    int dx, int dy) const noexcept
{
  int best = -1, best_distance = 0;

  for (unsigned i = 0; i < layout->count; ++i) {
    const PixelPoint p = layout->positions[i].GetCenter();
    const int along = (p.x - origin.x) * dx + (p.y - origin.y) * dy;
    if (along <= 0)
      continue;

    /* the closest slot in that direction wins, and among those the
       one which is least off to the side */
    const int across = std::abs((p.x - origin.x) * dy
                                - (p.y - origin.y) * dx);
    const int distance = along + 4 * across;

    if (best < 0 || distance < best_distance) {
      best = i;
      best_distance = distance;
    }
  }

  return best;
}

bool
InfoBoxArrangeWindow::HasEnabledButton() const noexcept
{
  for (const auto &i : buttons)
    if (i.enabled)
      return true;

  return false;
}

bool
InfoBoxArrangeWindow::IsButtonRowCloser(PixelPoint origin, int slot,
                                        bool forward) const noexcept
{
  if (!HasEnabledButton())
    return false;

  const int buttons_along =
    Along(GetButtonBlockRect().GetCenter()) - Along(origin);
  if (forward ? buttons_along <= 0 : buttons_along >= 0)
    return false;

  if (slot < 0)
    return true;

  return std::abs(buttons_along) <
    std::abs(Along(SlotCenter(slot)) - Along(origin));
}

bool
InfoBoxArrangeWindow::FocusButtonFrom(int dx, int dy) noexcept
{
  const int button = columns
    /* the cursor arrives from the side and takes the button at that
       end of the bottom row */
    ? FindButtonAt(0, dx > 0 ? 0. : 1.)
    /* it arrives from above or from below and keeps the place it
       remembers */
    : FindButtonAt(dy > 0 ? GetButtonRowCount() - 1 : 0, cross_fraction);

  if (button < 0)
    return false;

  focused_button = button;
  return true;
}

void
InfoBoxArrangeWindow::RememberCross() noexcept
{
  if (focused_button >= 0)
    cross_fraction = GetButtonCrossFraction(focused_button);
  else if (described_slot >= 0)
    cross_fraction = GetSlotFraction(described_slot);
}

bool
InfoBoxArrangeWindow::MoveFromButton(int dx, int dy, bool along) noexcept
{
  /* left and right switch between the buttons of a row, up and down
     between the rows, where the button keeps its place in the row */
  const unsigned row = buttons[focused_button].row;
  int button = -1;

  if (dx != 0)
    button = FindNextInRow(focused_button, dx);
  else if (dy < 0 ? row + 1 < GetButtonRowCount() : row > 0)
    /* when the button rows follow the axis, the cursor keeps the
       place it remembers; when they lie across it, it keeps its
       place in the row */
    button = FindButtonAt(dy < 0 ? row + 1 : row - 1,
                          columns
                          ? GetButtonFraction(focused_button)
                          : cross_fraction);

  if (button >= 0) {
    focused_button = button;

    if (!along)
      RememberCross();
  } else if (!along)
    return true;
  else {
    /* the cursor returns to the InfoBoxes at the place it remembers */
    const auto group =
      FindSlotGroup(Along(GetButtonBlockRect().GetCenter()), dx + dy);
    if (!group)
      return true;

    described_slot = FindSlotAt(*group, cross_fraction);
    focused_button = -1;
  }

  selection = Selection::FOCUSED;
  Invalidate();
  return true;
}

InfoBoxArrangeWindow::TabKey
InfoBoxArrangeWindow::GetTabKey(unsigned i) const noexcept
{
  const unsigned count = layout->count;

  if (i < count) {
    const PixelPoint p = layout->positions[i].GetCenter();
    return {Along(p), 0, Across(p)};
  }

  const unsigned button = i - count;
  const PixelPoint p = GetButtonRect(button).GetCenter();

  /* when the InfoBoxes stand in rows, every button row is a group of
     its own, between the InfoBoxes above and below it; when they
     stand in columns, the whole button block is one group between
     them, and inside it the rows are read from top to bottom, each
     from left to right */
  return columns
    ? TabKey{Along(GetButtonBlockRect().GetCenter()),
             int(GetButtonRowCount() - 1 - buttons[button].row), p.x}
    : TabKey{Along(p), 0, Across(p)};
}

void
InfoBoxArrangeWindow::FocusItem(unsigned i) noexcept
{
  const unsigned count = layout->count;

  if (i < count) {
    described_slot = i;
    focused_button = -1;
  } else {
    focused_button = i - count;
  }

  RememberCross();

  selection = Selection::FOCUSED;
  Invalidate();
}

bool
InfoBoxArrangeWindow::MoveTab(bool forward) noexcept
{
  OnArrangeActivity();

  if (selection == Selection::MOVING)
    /* a taken InfoBox is moved with the cursor keys */
    return true;

  const unsigned slot_count = layout->count;
  const unsigned current = focused_button >= 0
    ? slot_count + focused_button
    : (described_slot >= 0 ? unsigned(described_slot) : 0);
  const TabKey key = GetTabKey(current);

  int next = -1, wrap = -1;
  TabKey next_key{}, wrap_key{};

  for (unsigned i = 0; i < slot_count + buttons.size(); ++i) {
    if (i == current)
      continue;

    if (i >= slot_count && !buttons[i - slot_count].enabled)
      /* a disabled button is skipped */
      continue;

    const TabKey k = GetTabKey(i);

    if (forward ? key < k : k < key) {
      if (next < 0 || (forward ? k < next_key : next_key < k)) {
        next = i;
        next_key = k;
      }
    } else if (wrap < 0 || (forward ? k < wrap_key : wrap_key < k)) {
      wrap = i;
      wrap_key = k;
    }
  }

  const int item = next >= 0 ? next : wrap;
  if (item >= 0)
    FocusItem(item);

  return true;
}

bool
InfoBoxArrangeWindow::CanMoveSelection(int dx, int dy) const noexcept
{
  if (style == Style::MAP)
    /* nothing else on the map can have the focus, so the edge of the
       layout is a wall */
    return true;

  if (selection == Selection::MOVING)
    /* the InfoBox the user carries must not be left behind */
    return true;

  if (described_slot < 0 && focused_button < 0)
    /* the first key press selects the first InfoBox */
    return true;

  const bool along = columns ? dx != 0 : dy != 0;

  if (focused_button >= 0)
    /* the cursor moves inside the button block or leaves it */
    return true;

  const PixelPoint origin = SlotCenter(described_slot);
  const int slot = FindNeighbour(origin, dx, dy);

  return slot >= 0 || (along && IsButtonRowCloser(origin, slot, dx + dy > 0));
}

bool
InfoBoxArrangeWindow::MoveSelection(int dx, int dy) noexcept
{
  OnArrangeActivity();

  if (described_slot < 0 && focused_button < 0) {
    described_slot = 0;
    RememberCross();
    selection = Selection::FOCUSED;
    Invalidate();
    return true;
  }

  if (selection == Selection::MOVING) {
    /* a taken InfoBox stays among the slots; it must not land on a
       button */
    const int slot = FindNeighbour(SlotCenter(described_slot), dx, dy);
    if (slot < 0)
      return true;

    /* carry the InfoBox along, exchanging it with the neighbour; both
       slide into their new slot */
    Exchange(described_slot, slot);
    StartShuffle(described_slot, layout->positions[slot]);
    StartShuffle(slot, layout->positions[described_slot]);

    described_slot = slot;

    /* the cursor sits on the InfoBox it carries, so it leaves the
       layout where that one has landed */
    RememberCross();

    Invalidate();
    return true;
  }

  /* the cursor moves along the axis the InfoBoxes are stacked on:
     down when they stand in rows, to the side when they stand in
     columns.  The button row takes its place in that order by where
     it sits on the screen, between the InfoBoxes before and after
     it */
  const bool along = columns ? dx != 0 : dy != 0;

  if (focused_button >= 0)
    return MoveFromButton(dx, dy, along);

  const PixelPoint origin = SlotCenter(described_slot);
  const int slot = FindNeighbour(origin, dx, dy);

  if (along && IsButtonRowCloser(origin, slot, dx + dy > 0)) {
    if (!FocusButtonFrom(dx, dy))
      return true;
  } else if (along) {
    /* the cursor keeps its place inside the row (portrait) or column
       it moves to */
    const auto group = FindSlotGroup(Along(origin), dx + dy);
    if (!group)
      return true;

    described_slot = FindSlotAt(*group, cross_fraction);
  } else if (slot >= 0) {
    described_slot = slot;
    RememberCross();
  } else
    return true;

  selection = Selection::FOCUSED;
  Invalidate();
  return true;
}

bool
InfoBoxArrangeWindow::Activate() noexcept
{
  OnArrangeActivity();

  if (focused_button >= 0) {
    OnButton(focused_button);
    return true;
  }

  if (described_slot < 0)
    return true;

  if (selection != Selection::MOVING) {
    grab_slot = described_slot;
    selection = Selection::MOVING;
    Invalidate();
    return true;
  }

  const bool unmoved = (unsigned)described_slot == grab_slot;
  selection = Selection::FOCUSED;

  /* the numbers travelled with the InfoBoxes while one was carried;
     now they belong to their slots again */
  ResetCardNumbers();

  Invalidate();

  if (unmoved)
    /* taking and putting down without moving means the user wants to
       change the InfoBox instead */
    ShowPicker(described_slot);

  return true;
}

/*
 * the buttons
 */

ButtonState
InfoBoxArrangeWindow::GetButtonState(int i) const noexcept
{
  if (!buttons[i].enabled)
    return ButtonState::DISABLED;

  if (held_button == i && button_down)
    return ButtonState::PRESSED;

  if (focused_button == i)
    return ButtonState::FOCUSED;

  return ButtonState::ENABLED;
}

void
InfoBoxArrangeWindow::OnButton(int i) noexcept
{
  if (buttons[i].enabled)
    buttons[i].callback();
}

void
InfoBoxArrangeWindow::HoldButton(int i, bool down) noexcept
{
  if (i == held_button && down == button_down)
    return;

  held_button = i;
  button_down = down;
  Invalidate();
}

void
InfoBoxArrangeWindow::ReleaseButton() noexcept
{
  if (held_button < 0)
    return;

  held_button = -1;
  button_down = false;
  Invalidate();
}

/*
 * dragging
 */

void
InfoBoxArrangeWindow::FocusSlot(unsigned slot) noexcept
{
  described_slot = slot;
  focused_button = -1;
  RememberCross();
  selection = Selection::FOCUSED;
  Invalidate();
}

void
InfoBoxArrangeWindow::BeginDrag(unsigned slot, PixelPoint pointer,
                                bool follow) noexcept
{
  drag.emplace(DragState{slot, layout->positions[slot], pointer, pointer,
                         follow});
  drag_snapshot = *panel;
  ResetCardNumbers();

  /* the card the user has grabbed is the one being described; that
     way only ever one card is highlighted */
  described_slot = slot;
  selection = Selection::TOUCH;
  focused_button = -1;

  card_floating = follow;

  if (!follow)
    /* holding the InfoBox still opens the picker */
    picker_timer.Schedule(PICKER_DELAY);

  SetCapture();
  OnArrangeActivity();
  Invalidate();
}

void
InfoBoxArrangeWindow::Drag(PixelPoint p) noexcept
{
  if (!drag)
    return;

  drag->pointer = p;

  if (!drag->following) {
    const auto d = p - drag->origin;
    if (std::abs(d.x) + std::abs(d.y) < GetDeadZone())
      return;

    drag->following = true;
    card_floating = true;
    picker_timer.Cancel();
  }

  /* exchange with the slot the finger has moved into, so that a drag
     across several slots shifts them one by one instead of swapping
     with the slot it started from */
  const int slot = FindSlot(p);
  if (slot >= 0 && unsigned(slot) != drag->slot) {
    Exchange(drag->slot, slot);

    /* the InfoBox which was in the way slides over to the slot the
       dragged one has just left; the dragged one needs no animation
       because it follows the finger */
    StartShuffle(drag->slot, layout->positions[slot]);

    drag->slot = described_slot = slot;
  }

  Invalidate();

  if (style == Style::DIALOG)
    /* the card is not clipped to this window, so the main window has
       to clear what it leaves outside the dialog */
    UIGlobals::GetMainWindow().Invalidate();
}

void
InfoBoxArrangeWindow::Drop() noexcept
{
  if (!drag)
    return;

  picker_timer.Cancel();
  drag.reset();
  card_floating = false;
  ResetCardNumbers();
  RememberCross();
  ReleaseCapture();
  OnArrangeActivity();
  Invalidate();
}

void
InfoBoxArrangeWindow::CancelDrag() noexcept
{
  if (!drag)
    return;

  picker_timer.Cancel();
  drag.reset();
  card_floating = false;
  *panel = drag_snapshot;
  ResetCardNumbers();
  described_slot = -1;

  for (auto &i : shuffle)
    i.start = {};

  OnArrangeModified();
  Invalidate();
}

/*
 * input
 */

bool
InfoBoxArrangeWindow::OnMouseDown(PixelPoint p) noexcept
{
  OnArrangeActivity();

  const PixelPoint parent_p = ToParentCoordinates(p);

  if (const int button = FindButton(parent_p); button >= 0)
    HoldButton(button, true);
  else if (const int slot = FindSlot(parent_p); slot >= 0)
    BeginDrag(slot, parent_p, false);

  return true;
}

bool
InfoBoxArrangeWindow::OnMouseMove(PixelPoint p,
                                  [[maybe_unused]] unsigned keys) noexcept
{
  const PixelPoint parent_p = ToParentCoordinates(p);

  if (held_button >= 0)
    HoldButton(held_button, GetButtonRect(held_button).Contains(parent_p));
  else
    Drag(parent_p);

  return true;
}

bool
InfoBoxArrangeWindow::OnMouseUp([[maybe_unused]] PixelPoint p) noexcept
{
  if (held_button >= 0) {
    const int button = held_button;
    const bool clicked = button_down;
    ReleaseButton();

    if (clicked)
      OnButton(button);

    return true;
  }

  if (!drag)
    return true;

  Drop();
  return true;
}

bool
InfoBoxArrangeWindow::OnMouseDouble([[maybe_unused]] PixelPoint p) noexcept
{
  /* arranging InfoBoxes has no use for the menu */
  return true;
}

#ifdef HAVE_MULTI_TOUCH

bool
InfoBoxArrangeWindow::OnMultiTouchDown() noexcept
{
  /* a second finger aborts the drag */
  CancelDrag();
  ReleaseCapture();
  return true;
}

#endif

bool
InfoBoxArrangeWindow::OnKeyCheck(unsigned key_code) const noexcept
{
  switch (key_code) {
  case KEY_UP:
    return CanMoveSelection(0, -1);

  case KEY_DOWN:
    return CanMoveSelection(0, 1);

  case KEY_LEFT:
  case KEY_RIGHT:
  case KEY_RETURN:
  case KEY_ESCAPE:
  case KEY_TAB:
#ifdef USE_X11
  case XK_ISO_Left_Tab:
#endif
    return true;

  default:
    return false;
  }
}

bool
InfoBoxArrangeWindow::OnKeyDown(unsigned key_code) noexcept
{
  switch (key_code) {
  case KEY_LEFT:
    return MoveSelection(-1, 0);

  case KEY_RIGHT:
    return MoveSelection(1, 0);

  case KEY_UP:
    return MoveSelection(0, -1);

  case KEY_DOWN:
    return MoveSelection(0, 1);

  case KEY_TAB:
    return MoveTab(!IsShiftKeyPressed());

#ifdef USE_X11
  case XK_ISO_Left_Tab:
    /* X11 has its own key symbol for Shift+Tab */
    return MoveTab(false);
#endif

  case KEY_RETURN:
    return Activate();

  case KEY_ESCAPE:
    return OnArrangeCancel();
  }

  return PaintWindow::OnKeyDown(key_code);
}

void
InfoBoxArrangeWindow::OnCancelMode() noexcept
{
  ReleaseButton();
  CancelDrag();
  PaintWindow::OnCancelMode();
}
