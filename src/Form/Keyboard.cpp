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

#include "Form/Keyboard.hpp"
#include "Look/DialogLook.hpp"
#include "Util/StringUtil.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/ButtonWindow.hpp"
#include "Screen/Layout.hpp"

#include <assert.h>
#include <string.h>

static constexpr TCHAR keyboard_letters[] =
  _T("1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ");

KeyboardControl::KeyboardControl(ContainerWindow &parent,
                                 const DialogLook &_look,
                                 PixelRect rc,
                                 OnCharacterCallback_t _on_character,
                                 const WindowStyle _style)
  :look(_look),
   on_character(_on_character),
   num_buttons(0)
{
  Create(parent, rc, _style);

  TCHAR caption[] = _T(" ");

  for (const TCHAR *i = keyboard_letters; !StringIsEmpty(i); ++i) {
    caption[0] = *i;

    AddButton(caption);
  }

  AddButton(_T(" Space "));
  AddButton(_T("."));
  AddButton(_T(","));
  AddButton(_T("-"));

  MoveButtons();
}

void
KeyboardControl::SetAllowedCharacters(const TCHAR *allowed)
{
  for (unsigned i = 0; i < num_buttons; ++i)
    buttons[i].SetVisible(allowed == NULL ||
                          _tcschr(allowed, button_values[i]) != NULL);
}

ButtonWindow *
KeyboardControl::FindButton(TCHAR ch)
{
  for (unsigned i = 0; i < num_buttons; ++i)
    if (button_values[i] == ch)
      return &buttons[i];

  return NULL;

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
KeyboardControl::MoveButton(TCHAR ch, PixelScalar left, PixelScalar top)
{
  ButtonWindow *kb = FindButton(ch);
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
KeyboardControl::ResizeButton(TCHAR ch,
                              UPixelScalar width, UPixelScalar height)
{
  ButtonWindow *kb = FindButton(ch);
  if (kb)
    kb->Resize(width, height);
}

void
KeyboardControl::ResizeButtons()
{
  for (unsigned i = 0; i < num_buttons; ++i)
    buttons[i].Resize(button_width, button_height);
}

/**
 * Inicialize the button_width and button_height values.
 *
 * button_width is computed as width of the whole keyboard / 10
 * button_height is computed as height of the whole keyboard / 5
 *
 */
void
KeyboardControl::SetButtonsSize()
{
  button_width = GetWidth() / 10;
  button_height = GetHeight() / 5;
}

void
KeyboardControl::MoveButtonsToRow(const TCHAR* buttons, int row,
                                  PixelScalar offset)
{
  if (StringIsEmpty(buttons))
    return;

  ButtonWindow *kb;
  for (unsigned i = 0; buttons[i] != _T('\0'); i++) {
    kb = FindButton(buttons[i]);
    if (!kb)
      continue;

    kb->Move(i * button_width + offset, row * button_height);
  }
}

void
KeyboardControl::MoveButtons()
{
  MoveButtonsToRow(_T("1234567890"), 0);
  MoveButtonsToRow(_T("QWERTYUIOP"), 1);
  MoveButtonsToRow(_T("ASDFGHJKL"), 2, button_width / 3);
  MoveButtonsToRow(_T("ZXCVBNM,."), 3, button_width * 2 / 3);

  if (IsLandscape()) {
    MoveButton(_T('-'), button_width * 9, Layout::Scale(160));

    MoveButton(_T(' '), Layout::Scale(80), Layout::Scale(160));
    ResizeButton(_T(' '), Layout::Scale(93), Layout::Scale(40));
  } else {
    MoveButton(_T('-'), button_width * 8, button_height * 4);

    MoveButton(_T(' '), button_width * 2, button_height * 4);
    ResizeButton(_T(' '), button_width * 11 / 2, button_height);
  }
}

void
KeyboardControl::OnPaint(Canvas &canvas)
{
  canvas.Clear(look.background_color);

  ContainerWindow::OnPaint(canvas);
}

bool
KeyboardControl::OnCommand(unsigned id, unsigned code)
{
  if (id >= 0x20 && on_character != NULL) {
    on_character((TCHAR)id);
    return true;
  } else
    return ContainerWindow::OnCommand(id, code);
}

void
KeyboardControl::OnResize(UPixelScalar width, UPixelScalar height)
{
  SetButtonsSize();
  ResizeButtons();
  MoveButtons();
}

void
KeyboardControl::AddButton(const TCHAR *caption)
{
  assert(num_buttons < MAX_BUTTONS);

  button_values[num_buttons] = caption[0];

  PixelRect rc;
  rc.left = 0;
  rc.top = 0;
  rc.right = button_width;
  rc.bottom = button_height;

  ButtonWindow *button = &buttons[num_buttons++];
  button->Create(*this, caption, (unsigned)caption[0], rc);
  button->SetFont(*look.button.font);
}
