// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "KeyboardWidget.hpp"
#include "Renderer/SymbolButtonRenderer.hpp"
#include "Asset.hpp"
#include "util/StringAPI.hxx"
#include "util/StringCompare.hxx"
#include "util/CharUtil.hxx"
#include "ui/event/KeyCode.hpp"
#include "ui/window/ContainerWindow.hpp"
#include "ui/window/Window.hpp"
#include "Screen/Layout.hpp"

#include <algorithm>
#include <cassert>
#include <climits>
#include <cstdlib>
#include <string.h>
#include <utility>

namespace {

static constexpr char KEYBOARD_LETTERS[] =
  "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ";

/** Digits 0-9; matches first keys in @c KEYBOARD_LETTERS order. */
static constexpr int NUMBER_ROW_KEY_COUNT = 10;

static constexpr int DEFAULT_NUMBER_ROW_PREFER = 4; /* '5' */

static bool
ArrowToDirection(unsigned key_code, int &dix, int &diy) noexcept
{
  dix = diy = 0;
  switch (key_code) {
  case KEY_LEFT: dix = -1; return true;
  case KEY_RIGHT: dix = 1; return true;
  case KEY_UP: diy = -1; return true;
  case KEY_DOWN: diy = 1; return true;
  default: return false;
  }
}

} // namespace

void
KeyboardWidget::Unprepare() noexcept
{
  parent_container = nullptr;
  number_row_before_backspace = -1;
}

void
KeyboardWidget::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  parent_container = &parent;

  PrepareSize(rc);

  char caption[] = " ";

  for (const char *i = KEYBOARD_LETTERS; !StringIsEmpty(i); ++i) {
    caption[0] = *i;
    AddButton(parent, caption, *i);
  }

  AddButton(parent, "Space", ' ');
  AddButton(parent, ".", '.');
  AddButton(parent, "@", '@');
  AddButton(parent, "-", '-');

  if (show_shift_button) {
    WindowStyle style;
    style.Hide();
    style.TabStop();
    shift_button.Create(parent, { 0, 0, 16, 16 }, style,
                        std::make_unique<SymbolButtonRenderer>(look, "v"),
                        [this](){ OnShiftClicked(); });
  }
  UpdateShiftState();
}

void
KeyboardWidget::Show(const PixelRect &rc) noexcept
{
  OnResize(rc);

  for (unsigned i = 0; i < num_buttons; ++i)
    buttons[i].Show();

  if (show_shift_button)
    shift_button.Show();
}

void
KeyboardWidget::Hide() noexcept
{
  for (unsigned i = 0; i < num_buttons; ++i)
    buttons[i].Hide();

  if (show_shift_button)
    shift_button.Hide();
}

void
KeyboardWidget::Move(const PixelRect &rc) noexcept
{
  OnResize(rc);
}

void
KeyboardWidget::SetAllowedCharacters(const char *allowed)
{
  for (unsigned i = 0; i < num_buttons; ++i)
    buttons[i].SetEnabled(allowed == nullptr ||
                          StringFind(allowed, buttons[i].GetCharacter()) !=
                            nullptr);
}

bool
KeyboardWidget::FocusSpaceKey() noexcept
{
  if (Button *b = FindButton(' ');
      b != nullptr && b->IsEnabled() && b->IsVisible()) {
    b->SetFocus();
    return true;
  }
  return false;
}

Button *
KeyboardWidget::FindButton(unsigned ch)
{
  for (unsigned i = 0; i < num_buttons; ++i)
    if (buttons[i].GetUpperCharacter() == ch)
      return &buttons[i];

  return nullptr;
}

void
KeyboardWidget::MoveButton(unsigned ch, PixelPoint position) noexcept
{
  if (Button *b = FindButton(ch))
    b->Move(position);
}

void
KeyboardWidget::ResizeButton(unsigned ch, PixelSize size) noexcept
{
  if (Button *b = FindButton(ch))
    b->Resize(size);
}

void
KeyboardWidget::ResizeButtons()
{
  for (unsigned i = 0; i < num_buttons; ++i)
    buttons[i].Resize(button_size);

  if (show_shift_button)
    shift_button.Resize(button_size);
}

void
KeyboardWidget::MoveButtonsToRow(const PixelRect &rc, const char *row_keys,
                                 unsigned row, int offset) noexcept
{
  if (StringIsEmpty(row_keys))
    return;

  for (unsigned i = 0; row_keys[i] != '\0'; ++i) {
    MoveButton(row_keys[i],
               rc.GetTopLeft() + PixelSize(i * button_size.width + offset,
                                           int(row) * button_size.height));
  }
}

void
KeyboardWidget::MoveButtons(const PixelRect &rc)
{
  MoveButtonsToRow(rc, "1234567890", 0, 0);
  MoveButtonsToRow(rc, "QWERTYUIOP", 1, 0);
  MoveButtonsToRow(rc, "ASDFGHJKL", 2, int(button_size.width / 3));
  MoveButtonsToRow(rc, "ZXCVBNM@.", 3, int(button_size.width));

  if (IsLandscape(rc)) {
    MoveButton('-', rc.GetTopLeft() + PixelSize(int(button_size.width) * 9,
                                                int(Layout::Scale(160U))));
    MoveButton(' ', rc.GetTopLeft() + PixelSize(int(Layout::Scale(80U)),
                                                 int(Layout::Scale(160U))));
    ResizeButton(' ', {Layout::Scale(93U), Layout::Scale(40U)});
  } else {
    const int h = int(button_size.height);
    const int w = int(button_size.width);
    MoveButton('-', rc.GetTopLeft() + PixelSize(w * 8, h * 4));
    MoveButton(' ', rc.GetTopLeft() + PixelSize(w * 2, h * 4));
    ResizeButton(' ', {button_size.width * 11 / 2, button_size.height});
  }

  if (show_shift_button) {
    shift_button.Move(
      rc.GetTopLeft() + PixelSize{0U, 3U * (unsigned)button_size.height});
  }
}

void
KeyboardWidget::PrepareSize(const PixelRect &rc) noexcept
{
  const PixelSize new_size = rc.GetSize();
  button_size = {new_size.width / 10, new_size.height / 5};
}

void
KeyboardWidget::OnResize(const PixelRect &rc)
{
  PrepareSize(rc);
  ResizeButtons();
  MoveButtons(rc);
}

void
KeyboardWidget::AddButton(ContainerWindow &parent, const char *caption,
                          unsigned ch)
{
  assert(num_buttons < MAX_BUTTONS);

  WindowStyle style;
  style.Hide();
  style.TabStop();

  CharacterButton &b = buttons[num_buttons++];
  b.Create(parent, look, caption, PixelRect{button_size},
           on_character, ch, style);
}

void
KeyboardWidget::UpdateShiftState() noexcept
{
  if (show_shift_button)
    shift_button.SetCaption(shift_state ? "v" : "^");

  for (unsigned i = 0; i < num_buttons; ++i) {
    unsigned uch = buttons[i].GetCharacter();
    if (uch < 0x80) {
      const char c = char(uch);
      if (shift_state) {
        if (IsLowerAlphaASCII(c))
          buttons[i].SetCharacter(c - 0x20);
      } else {
        if (IsUpperAlphaASCII(c))
          buttons[i].SetCharacter(c + 0x20);
      }
    }
  }
}

void
KeyboardWidget::OnShiftClicked() noexcept
{
  assert(show_shift_button);

  shift_state = !shift_state;
  UpdateShiftState();
}

const Button *
KeyboardWidget::GetByIndex(int idx) const noexcept
{
  if (idx >= 0 && (unsigned)idx < num_buttons)
    return &buttons[(unsigned)idx];
  if (show_shift_button && idx == (int)num_buttons)
    return &shift_button;
  return nullptr;
}

Button *
KeyboardWidget::GetByIndex(int idx) noexcept
{
  return const_cast<Button *>(
    std::as_const(*this).GetByIndex(idx));
}

int
KeyboardWidget::FindIndexOf(const Window *w) const noexcept
{
  for (unsigned i = 0; i < num_buttons; ++i)
    if (w == (const Window *)&buttons[i])
      return (int)i;
  if (show_shift_button && w == (const Window *)&shift_button)
    return (int)num_buttons;
  return -1;
}

int
KeyboardWidget::GetSpaceKeyIndex() const noexcept
{
  for (unsigned i = 0; i < num_buttons; ++i) {
    if (buttons[i].GetCharacter() == (unsigned)' ')
      return (int)i;
  }
  return -1;
}

bool
KeyboardWidget::GetCenterByIndex(int idx, PixelPoint &out) const noexcept
{
  const Button *b = GetByIndex(idx);
  if (b == nullptr || !b->IsDefined() || !b->IsVisible())
    return false;
  const PixelRect r = b->GetPosition();
  out = {(r.left + r.right) / 2, (r.top + r.bottom) / 2};
  return true;
}

bool
KeyboardWidget::TryNavigableKeyCenter(int from_idx, int j, PixelPoint &c_out) const
  noexcept
{
  if (j == from_idx)
    return false;
  if (const Button *b = GetByIndex(j);
      b == nullptr || !b->IsEnabled() || !b->IsVisible())
    return false;
  return GetCenterByIndex(j, c_out);
}

bool
KeyboardWidget::TrySetFocusToEnabledKey(int j) noexcept
{
  if (Button *b = GetByIndex(j);
      b != nullptr && b->IsEnabled() && b->IsVisible()) {
    b->SetFocus();
    return true;
  }
  return false;
}

int
KeyboardWidget::FindIndexVerticalFrom(int from_idx, int diy) const noexcept
{
  assert(diy == 1 || diy == -1);

  PixelPoint cf;
  if (!GetCenterByIndex(from_idx, cf))
    return -1;

  const int n = GetNavigableCount();
  /* Next row in screen space (y grows downward), then closest on that
     row by @em x — pure geometry, not button / layout order. */
  const int y_band = (int)std::max(2U, button_size.height / 4U);

  int y_row_target;
  if (diy < 0) {
    y_row_target = INT_MIN;
    for (int j = 0; j < n; ++j) {
      PixelPoint cj;
      if (!TryNavigableKeyCenter(from_idx, j, cj))
        continue;
      if (cj.y < cf.y)
        y_row_target = std::max(y_row_target, cj.y);
    }
    if (y_row_target == INT_MIN)
      return -1;
  } else {
    y_row_target = INT_MAX;
    for (int j = 0; j < n; ++j) {
      PixelPoint cj;
      if (!TryNavigableKeyCenter(from_idx, j, cj))
        continue;
      if (cj.y > cf.y)
        y_row_target = std::min(y_row_target, cj.y);
    }
    if (y_row_target == INT_MAX)
      return -1;
  }

  const int space_idx = GetSpaceKeyIndex();
  const bool up_from_space = diy < 0 && from_idx == space_idx;

  int best = -1;
  int best_dx2 = INT_MAX;
  for (int j = 0; j < n; ++j) {
    /* From @em Space, the key above in @em y is the Z row; the shift
       key shares that band but is not the next “up” in the QWERTY
       path (Z → A → Q → row 0). */
    if (up_from_space && show_shift_button && j == (int)num_buttons)
      continue;
    PixelPoint cj;
    if (!TryNavigableKeyCenter(from_idx, j, cj))
      continue;
    if (std::abs(cj.y - y_row_target) > y_band)
      continue;
    if (diy < 0 && cj.y >= cf.y)
      continue;
    if (diy > 0 && cj.y <= cf.y)
      continue;
    const int dx = cj.x - cf.x;
    const int dx2 = dx * dx;
    if (dx2 < best_dx2 || (dx2 == best_dx2 && (best < 0 || j < best))) {
      best_dx2 = dx2;
      best = j;
    }
  }
  return best;
}

int
KeyboardWidget::FindIndexHorizontalFrom(int from_idx, int dix) const
  noexcept
{
  assert(dix == 1 || dix == -1);

  PixelPoint cf;
  if (!GetCenterByIndex(from_idx, cf))
    return -1;

  const int n = GetNavigableCount();
  /* Same key row by @em y only: QWERTY/ASDF are one row height apart, so
     @c height/2 is too loose (e.g. E can pick A). Use a tight band so we
     never “wrap” to the adjacent staggered row. */
  const int y_slop = (int)std::max(2U, button_size.height / 4U);
  int best = -1;

  if (dix < 0) {
    int best_x = INT_MIN;
    for (int j = 0; j < n; ++j) {
      PixelPoint cj;
      if (!TryNavigableKeyCenter(from_idx, j, cj))
        continue;
      if (std::abs(cj.y - cf.y) > y_slop)
        continue;
      if (cj.x >= cf.x)
        continue;
      if (cj.x > best_x) {
        best_x = cj.x;
        best = j;
      }
    }
  } else {
    int best_x = INT_MAX;
    for (int j = 0; j < n; ++j) {
      PixelPoint cj;
      if (!TryNavigableKeyCenter(from_idx, j, cj))
        continue;
      if (std::abs(cj.y - cf.y) > y_slop)
        continue;
      if (cj.x <= cf.x)
        continue;
      if (cj.x < best_x) {
        best_x = cj.x;
        best = j;
      }
    }
  }

  return best;
}

bool
KeyboardWidget::FocusFirstEnabledInNumberRow(int prefer_index) noexcept
{
  if (prefer_index >= 0 && prefer_index < NUMBER_ROW_KEY_COUNT &&
      TrySetFocusToEnabledKey(prefer_index))
    return true;
  for (int i = 0; i < NUMBER_ROW_KEY_COUNT; ++i) {
    if (TrySetFocusToEnabledKey(i))
      return true;
  }
  return false;
}

bool
KeyboardWidget::FocusFirstEnabledInGrid() noexcept
{
  const int n = GetNavigableCount();
  for (int j = 0; j < n; ++j) {
    if (TrySetFocusToEnabledKey(j))
      return true;
  }
  return false;
}

bool
KeyboardWidget::RouteSpaceToActionRow(unsigned key_code, Button *action_row,
                                      Window *w) noexcept
{
  if (action_row == nullptr || key_code != KEY_DOWN)
    return false;
  Button *const sp = FindButton(' ');
  if (sp == nullptr || w != static_cast<Window *>(sp))
    return false;
  action_row->SetFocus();
  return true;
}

bool
KeyboardWidget::RouteNumberRowAndBackspace(unsigned key_code, Button *back,
                                           Window *w) noexcept
{
  if (back == nullptr)
    return false;

  if (key_code == KEY_UP) {
    const int from = FindIndexOf(w);
    if (from >= 0 && from < NUMBER_ROW_KEY_COUNT) {
      number_row_before_backspace = from;
      back->SetFocus();
      return true;
    }
    return false;
  }

  if (key_code == KEY_DOWN && w == static_cast<Window *>(back)) {
    int p = number_row_before_backspace;
    if (p < 0 || p >= NUMBER_ROW_KEY_COUNT)
      p = DEFAULT_NUMBER_ROW_PREFER;
    if (FocusFirstEnabledInNumberRow(p))
      return true;
    /* e.g. allowed set enables only a letter: number row is all disabled. */
    return FocusFirstEnabledInGrid();
  }

  return false;
}

bool
KeyboardWidget::MoveByVerticalInGrid(int from, int diy, Button *back,
                                     Button *action_row_first) noexcept
{
  assert(diy == 1 || diy == -1);
  const int to = FindIndexVerticalFrom(from, diy);
  if (to >= 0)
    return TrySetFocusToEnabledKey(to);
  if (diy < 0 && back != nullptr) {
    /* No on-screen key @em up: back is the next step (inverse of
       @c down from that key into the grid, which uses the same
       @c FindIndexVerticalFrom with <tt>diy == 1</tt>). */
    number_row_before_backspace = -1;
    back->SetFocus();
    return true;
  }
  if (diy > 0 && action_row_first != nullptr) {
    /* No key below in the grid (e.g. all lower rows off): action row. */
    action_row_first->SetFocus();
    return true;
  }
  return false;
}

bool
KeyboardWidget::MoveFocusInGridByArrowKey(unsigned key_code, Window *w,
                                          Button *back,
                                          Button *action_row_first) noexcept
{
  int dix = 0, diy = 0;
  if (!ArrowToDirection(key_code, dix, diy))
    return false;

  const int from = FindIndexOf(w);
  if (from < 0)
    return false;
  if (dix == 0 && (diy == 1 || diy == -1))
    return MoveByVerticalInGrid(from, diy, back, action_row_first);
  if (dix != 0 && diy == 0) {
    const int to = FindIndexHorizontalFrom(from, dix);
    if (to < 0)
      return false;
    return TrySetFocusToEnabledKey(to);
  }
  return false;
}

bool
KeyboardWidget::KeyPressImpl(unsigned key_code, Button *backspace,
                             Button *action_row_first) noexcept
{
  if (!HasCursorKeys() || parent_container == nullptr)
    return false;
  Window *w = parent_container->GetFocusedWindow();
  if (w == nullptr)
    return false;

  if (action_row_first != nullptr &&
      w == static_cast<Window *>(action_row_first) && key_code == KEY_UP) {
    /* @em OK is outside the grid: first @em up is @em Space, then
       @c FindIndexVerticalFrom (Z → A → Q → 0…9) and
       @c RouteNumberRowAndBackspace to on-screen @em backspace. */
    return FocusSpaceKey();
  }

  if (RouteSpaceToActionRow(key_code, action_row_first, w))
    return true;
  if (backspace != nullptr && action_row_first != nullptr &&
      w == static_cast<Window *>(backspace) && key_code == KEY_UP) {
    /* @em backspace is not in the key grid, so
       @c MoveFocusInGridByArrowKey is never used; send @em up to @em OK. */
    action_row_first->SetFocus();
    return true;
  }
  if (RouteNumberRowAndBackspace(key_code, backspace, w))
    return true;
  return MoveFocusInGridByArrowKey(key_code, w, backspace, action_row_first);
}
