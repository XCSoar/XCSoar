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

#ifndef XCSOAR_KEYBOARD_CONTROL_HPP
#define XCSOAR_KEYBOARD_CONTROL_HPP

#include "Widget.hpp"
#include "Form/CharacterButton.hpp"
#include "Form/ActionListener.hpp"
#include "Form/Button.hpp"

#include <tchar.h>

struct ButtonLook;
class WndSymbolButton;

class KeyboardWidget : public NullWidget, ActionListener {
public:
  typedef bool (*OnCharacterCallback_t)(unsigned ch);

protected:
  enum Actions {
    SHIFT,
  };

  static constexpr unsigned MAX_BUTTONS = 40;

  const ButtonLook &look;

  OnCharacterCallback_t on_character;

  unsigned button_width;
  unsigned button_height;

  unsigned num_buttons;
  CharacterButton buttons[MAX_BUTTONS];

  Button shift_button;
  bool shift_state;

  const bool show_shift_button;

public:
  KeyboardWidget(const ButtonLook &_look,
                 OnCharacterCallback_t _on_character,
                 bool _show_shift_button,
                 bool _default_shift_state = true)
    :look(_look), on_character(_on_character), num_buttons(0),
     shift_state(_default_shift_state),
     show_shift_button(_show_shift_button) {}

  /**
   * Show only the buttons representing the specified character list.
   */
  void SetAllowedCharacters(const TCHAR *allowed);

private:
  void PrepareSize(const PixelRect &rc);
  void OnResize(const PixelRect &rc);

  gcc_pure
  Button *FindButton(unsigned ch);

  void MoveButton(unsigned ch, int left, int top);
  void ResizeButton(unsigned ch, unsigned width, unsigned height);
  void ResizeButtons();
  void SetButtonsSize();
  void MoveButtonsToRow(const PixelRect &rc,
                        const TCHAR *buttons, unsigned row,
                        int offset_left = 0);
  void MoveButtons(const PixelRect &rc);

  gcc_pure
  static bool IsLandscape(const PixelRect &rc) {
    return rc.GetWidth() >= rc.GetHeight();
  }

  /* updates UI based on value of shift_state property */
  void UpdateShiftState();

  void AddButton(ContainerWindow &parent, const TCHAR *caption, unsigned ch);

public:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) override;
  void Show(const PixelRect &rc) override;
  void Hide() override;
  void Move(const PixelRect &rc) override;

private:
  void OnShiftClicked();

  /* virtual methods from ActionListener */
  void OnAction(int id) override;
};

#endif
