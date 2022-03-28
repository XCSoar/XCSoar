/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "NumPadWidget.hpp"
#include "Renderer/SymbolButtonRenderer.hpp"
#include "util/StringAPI.hxx"
#include "util/StringCompare.hxx"
#include "util/CharUtil.hxx"
#include "Screen/Layout.hpp"
#include "Renderer/TextButtonRenderer.hpp"
#include "ui/window/ContainerWindow.hpp"
#include "Dialogs/DialogSettings.hpp"
#include "UIGlobals.hpp"

#include <cassert>
#include <string.h>

static constexpr long waitForSameKeyTime = 1000000;// one second = 1000.000 microseconds
static constexpr size_t MAX_TEXTENTRY = 40;
static constexpr size_t MAX_COLS = 3;
static constexpr size_t MAX_ROWS = 4;

void
NumPadWidget::Prepare(ContainerWindow &_parent, const PixelRect &rc) noexcept
{
  parent = &_parent;
  PrepareSize(rc);
//  for( int row= MAX_ROWS - 1;row>0;row--)
//    for( int col=0;col<MAX_COLS;col++)
  for (size_t row = 0; row < MAX_ROWS - 1; row++)
    for (size_t col = 0; col < 3; col++)
      AddButton(*parent, "TBD");
  AddButton(*parent, "TBD");

  if (show_shift_button) {
    WindowStyle style;
    style.Hide();
    shift_button.Create(*parent, { 0, 0, 16, 16 }, style,
                        std::make_unique<SymbolButtonRenderer>(look, _T("v")),
                        [this]
                        () {OnShiftClicked();});
  }

  UpdateShiftState();
  NumPadAdapter &a = GetNumPadWidgetInterface().GetNumPadAdapter();
  a.UpdateButtons();

  parent->GetFocusedWindow();
}

void
NumPadWidget::Show(const PixelRect &rc) noexcept
{
  OnResize(rc);

  for (unsigned i = 0; i < num_buttons; ++i)
    buttons[i].Show();

  if (show_shift_button)
    shift_button.Show();
}

void
NumPadWidget::Hide() noexcept
{
  for (unsigned i = 0; i < num_buttons; ++i)
    buttons[i].Hide();

  if (show_shift_button)
    shift_button.Hide();
}

void
NumPadWidget::Move(const PixelRect &rc) noexcept
{
  OnResize(rc);
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
NumPadWidget::MoveButton(unsigned idx, int left, int top)
{
  buttons[idx].Move(left, top);
}

void
NumPadWidget::ResizeButtons()
{
  for (unsigned i = 0; i < num_buttons; ++i)
    buttons[i].Resize(button_width, button_height);

  if (show_shift_button)
    shift_button.Resize(button_width, button_height);
}

void
NumPadWidget::MoveButtonsToRow(const PixelRect &rc, unsigned from, unsigned to,
                               unsigned row, int offset)
{
  for (unsigned i = from; i < to; ++i)
    MoveButton(i, rc.left + (i - from) * button_width + offset,
               rc.top + row * button_height);
}

void
NumPadWidget::MoveButtons(const PixelRect &rc)
{
  MoveButtonsToRow(rc, 0, 3, 0);
  MoveButtonsToRow(rc, 3, 6, 1);
  MoveButtonsToRow(rc, 6, 9, 2);
  MoveButtonsToRow(rc, 9, 10, 3);
  /*
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
   */
}

void
NumPadWidget::PrepareSize(const PixelRect &rc)
{
  const PixelSize new_size = rc.GetSize();
  button_width = new_size.width / 3;
  button_height = new_size.height / 4;
}

void
NumPadWidget::OnResize(const PixelRect &rc)
{
  PrepareSize(rc);
  ResizeButtons();
  MoveButtons(rc);
}
PixelRect
NumPadWidget::UpdateLayout() noexcept
{
  assert(parent != nullptr);
  return UpdateLayout(parent->GetClientRect());
}
PixelRect
NumPadWidget::UpdateLayout(PixelRect rc) noexcept
{
  return UpdateLayout(rc);
}

void
NumPadWidget::AddButton(ContainerWindow &parent, const TCHAR *caption)
{
  assert(num_buttons < MAX_BUTTONS);

  WindowStyle style;
  style.Hide();

  PixelRect rc;
  rc.left = 0;
  rc.top = 0;
  rc.right = button_width;
  rc.bottom = button_height;

  Button &button = buttons[num_buttons++];
  button.Create(parent, look, caption, rc, style);
}

void
NumPadWidget::UpdateShiftState()
{

#ifdef TODO
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
#endif
}

void
NumPadWidget::OnShiftClicked()
{

//  assert(show_shift_button);

//  shift_state = !shift_state;
//  UpdateShiftState();
}
bool
NumPadWidget::KeyPress(unsigned key_code) noexcept
{
// Handle keys when 1. The keyboard has a numpad or 2. It has the focus
  if (HasFocus()) {
    OnShiftClicked();
  }

  return false;
}

