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

#include "KeyboardWidget.hpp"
#include "Renderer/SymbolButtonRenderer.hpp"
#include "Util/StringAPI.hxx"
#include "Util/StringCompare.hxx"
#include "Util/CharUtil.hpp"
#include "Screen/Layout.hpp"

#include <assert.h>
#include <string.h>

static constexpr TCHAR keyboard_letters[] =
  _T("1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ");

void
KeyboardWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  PrepareSize(rc);

  TCHAR caption[] = _T(" ");

  for (const TCHAR *i = keyboard_letters; !StringIsEmpty(i); ++i) {
    caption[0] = *i;
    AddButton(parent, caption, *i);
  }

  AddButton(parent, _T("Space"), ' ');
  AddButton(parent, _T("."), '.');
  AddButton(parent, _T(","), ',');
  AddButton(parent, _T("-"), '-');

  if (show_shift_button) {
    WindowStyle style;
    style.Hide();
    shift_button.Create(parent, { 0, 0, 16, 16 }, style,
                        new SymbolButtonRenderer(look, _T("v")),
                        *this, SHIFT);
  }
  UpdateShiftState();
}

void
KeyboardWidget::Show(const PixelRect &rc)
{
  OnResize(rc);

  for (unsigned i = 0; i < num_buttons; ++i)
    buttons[i].Show();

  if (show_shift_button)
    shift_button.Show();
}

void
KeyboardWidget::Hide()
{
  for (unsigned i = 0; i < num_buttons; ++i)
    buttons[i].Hide();

  if (show_shift_button)
    shift_button.Hide();
}

void
KeyboardWidget::Move(const PixelRect &rc)
{
  OnResize(rc);
}

void
KeyboardWidget::SetAllowedCharacters(const TCHAR *allowed)
{
  for (unsigned i = 0; i < num_buttons; ++i)
    buttons[i].SetVisible(allowed == nullptr ||
                          StringFind(allowed, buttons[i].GetCharacter()) != nullptr);
}

Button *
KeyboardWidget::FindButton(unsigned ch)
{
  for (unsigned i = 0; i < num_buttons; ++i)
    if (buttons[i].GetUpperCharacter() == ch)
      return &buttons[i];

  return nullptr;

}

/**
 * Move button to the specified left and top coordinates.
 *
 * The coordinates SHOULD BE in pixels of the screen (i.e. after scaling!)
 *
 * @param ch
 * @param left    Number of pixels from the left (in screen pixels)
 * @param top     Number of pixels from the top (in screen pixels)
 */
void
KeyboardWidget::MoveButton(unsigned ch, int left, int top)
{
  auto *kb = FindButton(ch);
  if (kb)
    kb->Move(left, top);
}

/**
 * Resizes the button to specified width and height values according to display pixels!
 *
 *
 * @param ch
 * @param width   Width measured in display pixels!
 * @param height  Height measured in display pixels!
 */
void
KeyboardWidget::ResizeButton(unsigned ch,
                             unsigned width, unsigned height)
{
  auto *kb = FindButton(ch);
  if (kb)
    kb->Resize(width, height);
}

void
KeyboardWidget::ResizeButtons()
{
  for (unsigned i = 0; i < num_buttons; ++i)
    buttons[i].Resize(button_width, button_height);

  if (show_shift_button)
    shift_button.Resize(button_width, button_height);
}

void
KeyboardWidget::MoveButtonsToRow(const PixelRect &rc,
                                 const TCHAR *buttons, unsigned row,
                                 int offset)
{
  if (StringIsEmpty(buttons))
    return;

  for (unsigned i = 0; buttons[i] != _T('\0'); i++) {
    MoveButton(buttons[i],
               rc.left + i * button_width + offset,
               rc.top + row * button_height);
  }
}

void
KeyboardWidget::MoveButtons(const PixelRect &rc)
{
  MoveButtonsToRow(rc, _T("1234567890"), 0);
  MoveButtonsToRow(rc, _T("QWERTYUIOP"), 1);
  MoveButtonsToRow(rc, _T("ASDFGHJKL"), 2, button_width / 3);
  MoveButtonsToRow(rc, _T("ZXCVBNM,."), 3, button_width);

  if (IsLandscape(rc)) {
    MoveButton(_T('-'),
               rc.left + button_width * 9,
               rc.top + Layout::Scale(160));

    MoveButton(_T(' '),
               rc.left + Layout::Scale(80),
               rc.top + Layout::Scale(160));
    ResizeButton(_T(' '), Layout::Scale(93), Layout::Scale(40));
  } else {
    MoveButton(_T('-'),
               rc.left + button_width * 8,
               rc.top + button_height * 4);

    MoveButton(_T(' '),
               rc.left + button_width * 2,
               rc.top + button_height * 4);
    ResizeButton(_T(' '), button_width * 11 / 2, button_height);
  }

  if (show_shift_button)
    shift_button.Move(rc.left, rc.top + 3 * button_height);
}

void
KeyboardWidget::PrepareSize(const PixelRect &rc)
{
  const PixelSize new_size = rc.GetSize();
  button_width = new_size.cx / 10;
  button_height = new_size.cy / 5;
}

void
KeyboardWidget::OnResize(const PixelRect &rc)
{
  PrepareSize(rc);
  ResizeButtons();
  MoveButtons(rc);
}

void
KeyboardWidget::AddButton(ContainerWindow &parent,
                          const TCHAR *caption, unsigned ch)
{
  assert(num_buttons < MAX_BUTTONS);

  WindowStyle style;
  style.Hide();

  PixelRect rc;
  rc.left = 0;
  rc.top = 0;
  rc.right = button_width;
  rc.bottom = button_height;

  CharacterButton &button = buttons[num_buttons++];
  button.Create(parent, look, caption, rc, on_character, ch, style);
}

void
KeyboardWidget::UpdateShiftState()
{
  if (show_shift_button)
    shift_button.SetCaption(shift_state ? _T("v") : _T("^"));

  for (unsigned i = 0; i < num_buttons; ++i) {
    unsigned uch = buttons[i].GetCharacter();
    if (uch < 0x80) {
      char ch = char(uch);
      if (shift_state) {
        if (IsLowerAlphaASCII(ch))
          buttons[i].SetCharacter(ch - 0x20);
      } else {
        if (IsUpperAlphaASCII(ch))
          buttons[i].SetCharacter(ch + 0x20);
      }
    }
  }
}

void
KeyboardWidget::OnShiftClicked()
{
  assert(show_shift_button);

  shift_state = !shift_state;
  UpdateShiftState();
}

void
KeyboardWidget::OnAction(int id)
{
  switch (id) {
  case SHIFT:
    OnShiftClicked();
    break;
  }
}
