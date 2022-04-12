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

#ifndef XCSOAR_NUMPADWIDGET_HPP
#define XCSOAR_NUMPADWIDGET_HPP

#include "Form/DataField/NumPadAdapter.hpp"
#include "Form/DataField/NumPadWidgetInterface.hpp"
#include "Widget.hpp"
#include "Widget/TextListWidget.hpp"
#include "Form/CharacterButton.hpp"
#include "Form/Button.hpp"
#include "Form/NumPad.hpp"
#include "Form/Form.hpp"
#include "Dialogs/TextEntry.hpp"
#include "Dialogs/TextEntry.hpp"
#include "Dialogs/TextEntry.hpp"
#include "ui/window/ContainerWindow.hpp"
#include "ui/canvas/Bitmap.hpp"
#include "Form/DataField/ComboList.hpp"
#include <tchar.h>

struct ButtonLook;
class WndSymbolButton;

class NumPadWidget: public NullWidget, ListItemRenderer, ListCursorHandler
{
  const TCHAR *NUMPAD_CAPTION =  _T(",\nNum Pad");
  const TCHAR *END_CAPTION =  _T(",\nPad End");

  class MyTextListWidget: public WindowWidget
  {
  public:
// virtual from TextListWidget
    void
    SetItemRenderer(ListItemRenderer *_itemRenderer)
    {
      ((ListControl&)GetWindow()).SetItemRenderer(_itemRenderer);
    }
    void
    SetCursorHandler(ListCursorHandler *cursorHandler)
    {
      ((ListControl&)GetWindow()).SetCursorHandler(cursorHandler);
    }
    void
    SetCursorIndex(unsigned index)
    {
      ((ListControl&)GetWindow()).SetCursorIndex(index);
    }
    void
    SetLength(unsigned length)
    {
      ((ListControl&)GetWindow()).SetLength(length);
    }
    void
    Invalidate()
    {
      ((ListControl&)GetWindow()).Invalidate();
    }
  public:
    void
    Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  public:
    bool
    IsVisible() noexcept
    {
      return ((ListControl&)GetWindow()).IsVisible();
    }
    void
    ShowWindow() noexcept
    {
      ((ListControl&)GetWindow()).Show();
    }
  };

  enum Modes
  {
    Nothing,
    ListMode,
    ButtonMode
  };
public:
  typedef bool
  (*OnCharacterCallback_t)(unsigned ch);
protected:
  static constexpr unsigned MAX_BUTTONS = 10;

  const ButtonLook &look;

  OnCharacterCallback_t on_character;
  unsigned button_width;
  unsigned button_height;
  unsigned caption_height;

  unsigned num_buttons;
  Button buttons[MAX_BUTTONS];
  Bitmap calc;
  NumPad numPadWindow;
  MyTextListWidget textList;
  TextRowRenderer row_renderer;
  Button shiftButton;
  Button backspaceButton;
  Button editButton;
  bool shift_state;

  const bool show_shift_button;
  bool editMode;
  Window *previousFocusWindow;
public:
  NumPadWidget(WndForm &dialog,const ButtonLook &_look, bool _show_shift_button,
               bool _default_shift_state = true) : look(_look), on_character(
      nullptr), num_buttons(0), numPadWindow(numPadWidgetInterface), shift_state(
      _default_shift_state), show_shift_button(_show_shift_button), editMode(
      false)
  {
    WndForm::CharacterFunction f = std::bind(
        &NumPadWidgetInterface::CharacterFunction, &numPadWidgetInterface,
        std::placeholders::_1);

    dialog.SetCharacterFunction(f);
  }
  /**
   * Show only the buttons representing the specified character list.
   */
private:
  NumPadWidgetInterface numPadWidgetInterface;
  ContainerWindow *parent;
  void
  CheckKey(TCHAR *output, const TCHAR *allowedCharacters,
           const TCHAR key) noexcept;
  void
  PrepareSize(const PixelRect &rc, unsigned border);
  void
  OnResize(const PixelRect &rc);

  void
  MoveButton(unsigned ch, int left, int top);
  void
  ResizeButtons();
  void
  SetButtonsSize();
  void
  MoveButtonsToRow(const PixelRect &rc, unsigned from, unsigned to,
                   unsigned row, int offset);
  void
  MoveButtons(const PixelRect &rc, unsigned border);
  void
  SetListMode(Modes newMode) noexcept;
  [[gnu::pure]]
  static bool
  IsLandscape(const PixelRect &rc)
  {
    return rc.GetWidth() >= rc.GetHeight();
  }
  void
  UpdateShiftState();
  void
  AddButton(ContainerWindow &parent, unsigned buttonIndex);
  void
  AddNumPadWindow(ContainerWindow &parent);
  void
  AddTextListWindow(ContainerWindow &parent);
  Modes
  getMode();
public:
  void
  FocusParent() noexcept
  {
    if (parent != nullptr)
      parent->SetFocus();
  }
  unsigned
  GetNumButtons() noexcept
  {
    return num_buttons;
  }
  Button*
  GetButtons() noexcept
  {
    return buttons;
  }
  /* virtual methods from class Widget */
  void
  Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void
  Show(const PixelRect &rc) noexcept override;
  void
  Hide() noexcept override;
  void
  Move(const PixelRect &rc) noexcept override;
  virtual bool
  HasFocus() const noexcept override
  {
    return NullWidget::HasFocus();
  }

  void
  OnButton()
  {

  }
  NumPadWidgetInterface&
  GetNumPadWidgetInterface()
  {
    numPadWidgetInterface.SetNumPadWidget(this);
    return numPadWidgetInterface;
  }
  /*
   * Stores old Focus and sets Focus to the this Widget
   */
  void
  BeginEditing() noexcept
  {
    if (editButton.IsVisible())
      editButton.SetCaption(END_CAPTION);
  }
  void OnDataFieldSetFocus() noexcept;
  void
  SetCursorIndex(unsigned index)
  {
    textList.SetCursorIndex(index);
  }
  /*
   * Stores old Focus and sets Focus to the this Widget
   */
  void
  EndEditing()
  {
    if (editButton.IsVisible())
      editButton.SetCaption(NUMPAD_CAPTION);
  }
  /* updates UI based on value of shift_state property */
private:
  void
  OnShiftClicked();

  void
  OnPaintItem(Canvas &canvas, PixelRect rc, unsigned i) noexcept override
  {
    ComboList *cbl = GetNumPadWidgetInterface().GetNumPadAdapter().GetComboList();
    if (cbl != nullptr)
      row_renderer.DrawTextRow(canvas, rc, (*cbl)[i].display_string.c_str());
  }
  void
  OnCursorMoved([[maybe_unused]] unsigned index) noexcept override
  {
    GetNumPadWidgetInterface().GetNumPadAdapter().OnCursorMoved(index);
  }
};

#endif
