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
#include "Form/CharacterButton.hpp"
#include "Form/Button.hpp"
#include "Dialogs/TextEntry.hpp"
#include "Dialogs/TextEntry.hpp"
#include "Dialogs/TextEntry.hpp"
#include "ui/window/ContainerWindow.hpp"
#include <tchar.h>

struct ButtonLook;
class WndSymbolButton;

class NumPadWidget : public NullWidget {
public:
  typedef bool (*OnCharacterCallback_t)(unsigned ch);

protected:
  static constexpr unsigned MAX_BUTTONS = 10;

  const ButtonLook &look;

  OnCharacterCallback_t on_character;
  unsigned button_width;
  unsigned button_height;

  unsigned num_buttons;
  Button buttons[MAX_BUTTONS];

  Button shift_button;
  bool shift_state;

  const bool show_shift_button;
  bool editMode;
  Window *previousFocusWindow;
public:
  NumPadWidget(const ButtonLook &_look,
                 bool _show_shift_button,
                 bool _default_shift_state = true)
    :look(_look), on_character(nullptr), num_buttons(0),
     shift_state(_default_shift_state),
     show_shift_button(_show_shift_button),editMode(false) {
   }
  /**
   * Show only the buttons representing the specified character list.
   */

private:
  TCHAR UpdateAllowedCharacters() noexcept;
  AllowedCharacters NumPadAllowedCharactersCallback;
  NumPadWidgetInterface numPadWidgetInterface;
  ContainerWindow *parent;
  void CheckKey(TCHAR *output, const TCHAR *allowedCharacters,
                                  const TCHAR key) noexcept;
  void PrepareSize(const PixelRect &rc);
  void OnResize(const PixelRect &rc);


  void MoveButton(unsigned ch, int left, int top);
  void ResizeButtons();
  void SetButtonsSize();
  void MoveButtonsToRow(const PixelRect &rc,
                        unsigned from, unsigned to, unsigned row,
                        int offset_left = 0);
  void MoveButtons(const PixelRect &rc);
  PixelRect UpdateLayout(PixelRect rc) noexcept;
  PixelRect UpdateLayout() noexcept;
  [[gnu::pure]]
  static bool IsLandscape(const PixelRect &rc) {
    return rc.GetWidth() >= rc.GetHeight();
  }
  void UpdateShiftState();

  void AddButton(ContainerWindow &parent, const TCHAR *caption);

public:
  unsigned GetNumButtons() noexcept {
    return num_buttons;
  }
  Button *GetButtons() noexcept  {
    return buttons;
  }
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;
  void Move(const PixelRect &rc) noexcept override;
  bool KeyPress(unsigned key_code) noexcept;
  bool HasFocus() noexcept{
    return NullWidget::HasFocus();
  }

  NumPadWidgetInterface &GetNumPadWidgetInterface()
  {
    numPadWidgetInterface.SetNumPadWidget( this );
    return numPadWidgetInterface;
  }
  /*
   * Stores old Focus and sets Focus to the this Widget
   */
  void BeginEditing() noexcept{
    assert(parent != nullptr);
    previousFocusWindow = parent->GetFocusedWindow();
    SetFocus();
  }
  /*
   * Stores old Focus and sets Focus to the this Widget
   */
  void EndEditing(){
     assert(previousFocusWindow != nullptr);
     previousFocusWindow->SetFocus();
  }
  /* updates UI based on value of shift_state property */
private:
  void OnShiftClicked();
};

#endif
