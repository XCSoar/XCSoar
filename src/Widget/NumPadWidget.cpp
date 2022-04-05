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
#include "Renderer/BitmapButtonRenderer.hpp"
#include "util/StringAPI.hxx"
#include "util/StringCompare.hxx"
#include "util/CharUtil.hxx"
#include "Screen/Layout.hpp"
#include "Renderer/TextButtonRenderer.hpp"
#include "ui/window/ContainerWindow.hpp"
#include "Dialogs/DialogSettings.hpp"
#include "Form/DataField/EnumListNumPadAdapter.hpp"
#include "Form/DataField/TextNumPadAdapter.hpp"
#include "Look/DialogLook.hpp"
#include "Resources.hpp"
#include "ui/canvas/Icon.hpp"
#include "Look/IconLook.hpp"

#include "UIGlobals.hpp"
#include "LogFile.hpp"

#include <cassert>
#include <string.h>

static constexpr long waitForSameKeyTime = 1000000;// one second = 1000.000 microseconds
static constexpr size_t MAX_COLS = 3;
static constexpr size_t MAX_ROWS = 4;

class MaskedIconButtonRenderer: public ButtonRenderer
{
  ButtonFrameRenderer frame_renderer;
public:
  MaskedIconButtonRenderer(const ButtonLook &_look) : frame_renderer(
      _look)
  {
  };
  void
  DrawButton(Canvas &canvas, const PixelRect &rc,
             ButtonState state) const noexcept
  {
    frame_renderer.DrawButton(canvas, rc, state);
    PixelRect rc1 = rc;
    rc1.Grow(-10,-10);
    const MaskedIcon &ic= UIGlobals::GetIconLook().hBmpTabCalculator;
    ic.Draw(canvas, rc1, false);
  }
};
void
NumPadWidget::MyTextListWidget::Prepare(ContainerWindow &parent,
                                        const PixelRect &rc) noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  WindowStyle list_style;
  list_style.Hide();
  list_style.Border();
  unsigned row_height = 20;
  auto list = std::make_unique<ListControl>(parent, look, rc, list_style,
                                            row_height);
//    list->SetCursorHandler(this);
  SetWindow(std::move(list));

//  const DialogLook &look = UIGlobals::GetDialogLook();
//  CreateList(parent, look, rc,
//             row_renderer.CalculateLayout(*look.list.font));

}

NumPadWidget::Modes
NumPadWidget::getMode()
{
  if (dynamic_cast<const EnumListNumPadAdapter*>(&numPadWidgetInterface.GetNumPadAdapter()))
    return ListMode;
  if (dynamic_cast<const TextNumPadAdapter*>(&numPadWidgetInterface.GetNumPadAdapter()))
    return ButtonMode;
  return Nothing;
}
void
NumPadWidget::Prepare(ContainerWindow &_parent, const PixelRect &rc) noexcept
{
  parent = &_parent;
  PrepareSize(rc, 3);
//  for( int row= MAX_ROWS - 1;row>0;row--)
//    for( int col=0;col<MAX_COLS;col++)
  for (num_buttons = 0; num_buttons < MAX_BUTTONS; num_buttons++)
    AddButton(*parent, num_buttons);
  AddButton(*parent, NumPadWidgetInterface::SHIFT_INDEX);
  AddButton(*parent, NumPadWidgetInterface::BACKSPACE_INDEX);
  AddButton(*parent, NumPadWidgetInterface::EDIT_INDEX);

  UpdateShiftState();
  AddNumPadWindow(_parent);
  const DialogLook &look = UIGlobals::GetDialogLook();
  row_renderer.CalculateLayout(*look.list.font);
  textList.Prepare(_parent, rc);
  textList.Show();
  textList.SetItemRenderer(this);
  textList.SetCursorHandler(this);

  numPadWidgetInterface.GetNumPadAdapter().UpdateButtons();

  parent->GetFocusedWindow();
}

void
NumPadWidget::SetListMode(Modes newMode) noexcept
{
  Hide();
  bool buttonsVisible = (newMode == ButtonMode);
  if (buttonsVisible) {
    GetNumPadWidgetInterface().GetNumPadAdapter().UpdateButtons();
  } else
    textList.Show();
  if (buttonsVisible) {
    backspaceButton.Show();
    editButton.Show();

    if (show_shift_button)
      shiftButton.Show();
  }
  if (newMode != Nothing && !numPadWindow.IsVisible())
    numPadWindow.Show();
}

void
NumPadWidget::AddTextListWindow(ContainerWindow &parent)
{
}

void
NumPadWidget::Show(const PixelRect &rc) noexcept
{
  OnResize(rc);

  Modes newMode = getMode();
  bool buttonsVisible = (newMode == ButtonMode);

  if (buttonsVisible)
    for (unsigned i = 0; i < num_buttons; ++i)
      buttons[i].Show();

  if (buttonsVisible && show_shift_button)
    shiftButton.Show();
  if (buttonsVisible && !backspaceButton.IsVisible())
    backspaceButton.Show();
  if (buttonsVisible && !editButton.IsVisible())
    editButton.Show();

  if (newMode != Nothing && !numPadWindow.IsVisible())
    numPadWindow.Show();
  if (newMode == ListMode && !textList.IsVisible())
    textList.Show();
}

void
NumPadWidget::Hide() noexcept
{
  for (unsigned i = 0; i < num_buttons; ++i)
    if (buttons[i].IsVisible())
      buttons[i].Hide();

  if (show_shift_button && shiftButton.IsVisible())
    shiftButton.Hide();
  if (backspaceButton.IsVisible())
    backspaceButton.Hide();
  if (editButton.IsVisible())
    editButton.Hide();
  if (numPadWindow.IsVisible())
    numPadWindow.Hide();
  if (numPadWindow.IsVisible())
    numPadWindow.Hide();
  if (textList.IsVisible())
    textList.Hide();
  numPadWindow.Invalidate();
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
  for (unsigned i = 0; i < num_buttons - 1; ++i)
    buttons[i].Resize(button_width, button_height);
  buttons[9].Resize(2 * button_width, button_height);
  if (show_shift_button)
    shiftButton.Resize(button_width, button_height);
  backspaceButton.Resize(button_width, button_height);
  editButton.Resize(button_width, button_height);
}

void
NumPadWidget::MoveButtonsToRow(const PixelRect &rc, unsigned from, unsigned to,
                               unsigned row, int offset)
{
  unsigned rowPos = rc.top + caption_height + row * button_height;
  for (unsigned i = from; row < 3 && i < to; ++i) {
    MoveButton(i, rc.left + (i - from) * button_width + offset, rowPos);
  }
  if (row == 3) {
    MoveButton(9, rc.left + offset, rowPos);
    editButton.Move(rc.left + 2 * button_width + offset, rowPos);
  }
  if (row == 4) {
    if (show_shift_button)
      shiftButton.Move(rc.left + offset, rowPos);
    backspaceButton.Move(rc.left + button_width + offset, rowPos);
  }
}

void
NumPadWidget::MoveButtons(const PixelRect &rc, unsigned border)
{
  PixelRect captionRect = rc;
  PixelRect contentRect = rc;
  contentRect.left = captionRect.left += border;
  contentRect.right = captionRect.right -= border;
  contentRect.bottom = captionRect.bottom;
  captionRect.bottom = contentRect.top = rc.top + caption_height;

  numPadWindow.Move(captionRect);
  if (textList.IsVisible())
    textList.Move(contentRect);
  MoveButtonsToRow(rc, 0, 3, 0, border);
  MoveButtonsToRow(rc, 3, 6, 1, border);
  MoveButtonsToRow(rc, 6, 9, 2, border);
  MoveButtonsToRow(rc, 9, 10, 3, border);
  MoveButtonsToRow(rc, 0, 0, 4, border);
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
NumPadWidget::PrepareSize(const PixelRect &rc, unsigned border)
{
  const PixelSize new_size = rc.GetSize();

  button_width = (new_size.width - 2 * border) / 3;
  caption_height = new_size.height / 22;// 5 Rows (4 * caption) + 2 caption
  button_height = caption_height * 4;
  caption_height *= 2;
}

void
NumPadWidget::OnResize(const PixelRect &rc)
{
  unsigned border = 3;
  PrepareSize(rc, border);
  ResizeButtons();
  MoveButtons(rc, 3);
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
NumPadWidget::AddButton(ContainerWindow &parent, unsigned buttonIndex)
{
  WindowStyle style;
//style.Hide();
  PixelRect rc;
  rc.left = 0;
  rc.top = 0;
  rc.right = button_width;
  rc.bottom = button_height;
  switch (buttonIndex) {
  case NumPadWidgetInterface::SHIFT_INDEX:
    if (show_shift_button) {
      WindowStyle style;
      shiftButton.Create(
          parent,
          { 0, 0, 16, 16 },
          style,
          std::make_unique<SymbolButtonRenderer>(look, _T("v")),
          std::bind(&NumPadWidgetInterface::OnButton,
                    GetNumPadWidgetInterface(), buttonIndex));
    }

    break;
  case NumPadWidgetInterface::BACKSPACE_INDEX:
    backspaceButton.SetCallback(
        std::bind(&NumPadWidgetInterface::OnButton, GetNumPadWidgetInterface(),
                  buttonIndex));
    backspaceButton.Create(parent, look, _T("<-"), rc, style);
    break;
  case NumPadWidgetInterface::EDIT_INDEX: {
    WindowStyle style;
//    editButton.Create(
//        parent,
//        { 0, 0, 16, 16 },
//        style,
//        std::make_unique<MaskedIconButtonRenderer>(look),
//        std::bind(&NumPadWidgetInterface::OnButton, GetNumPadWidgetInterface(),
//                  buttonIndex));

    editButton.SetCallback(
        std::bind(&NumPadWidgetInterface::OnButton, GetNumPadWidgetInterface(),
                    buttonIndex));
    editButton.Create(parent, look, NUMPAD_CAPTION, rc, style);
  }
    break;
  default:
    assert(num_buttons < MAX_BUTTONS);
    Button &button = buttons[buttonIndex];
    button.SetCallback(
        std::bind(&NumPadWidgetInterface::OnButton, GetNumPadWidgetInterface(),
                  buttonIndex));
    button.Create(parent, look, _T(""), rc, style);
    break;
  }
}
void
NumPadWidget::AddNumPadWindow(ContainerWindow &parent)
{
  WindowStyle style;
//style.TabStop();

  PixelRect rc;
  rc.left = 0;
  rc.top = 0;
  rc.right = parent.GetWidth();
  rc.bottom = parent.GetHeight();
  const TCHAR *caption = numPadWidgetInterface.GetNumPadAdapter().GetCaption();
  numPadWindow.Create(parent, caption, rc, style);
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

