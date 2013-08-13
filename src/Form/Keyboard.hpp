/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Screen/ContainerWindow.hpp"
#include "Screen/ButtonWindow.hpp"

#include <tchar.h>

struct DialogLook;

class KeyboardControl : public ContainerWindow {
public:
  typedef bool (*OnCharacterCallback_t)(unsigned ch);

protected:
  enum {
    MAX_BUTTONS = 40,
  };

  const DialogLook &look;

  OnCharacterCallback_t on_character;

  UPixelScalar button_width;
  UPixelScalar button_height;

  unsigned num_buttons;
  ButtonWindow buttons[MAX_BUTTONS];
  unsigned button_values[MAX_BUTTONS];

public:
  KeyboardControl(ContainerWindow &parent, const DialogLook &look,
                  PixelRect rc,
                  OnCharacterCallback_t on_character,
                  const WindowStyle _style = WindowStyle());

  /**
   * Show only the buttons representing the specified character list.
   */
  void SetAllowedCharacters(const TCHAR *allowed);

  void SetOnCharacterCallback(OnCharacterCallback_t _on_character) {
    on_character = _on_character;
  }

protected:
  virtual void OnPaint(Canvas &canvas) override;
  virtual bool OnCommand(unsigned id, unsigned code) override;
  virtual void OnResize(PixelSize new_size) override;

private:
  gcc_pure
  ButtonWindow *FindButton(unsigned ch);

  void MoveButton(unsigned ch, PixelScalar left, PixelScalar top);
  void ResizeButton(unsigned ch, UPixelScalar width, UPixelScalar height);
  void ResizeButtons();
  void SetButtonsSize();
  void MoveButtonsToRow(const TCHAR *buttons, int row,
                        PixelScalar offset_left = 0);
  void MoveButtons();

  gcc_pure
  bool IsLandscape() const {
    return GetWidth() >= GetHeight();
  }

  void AddButton(const TCHAR *caption, unsigned ch);
};

#endif
