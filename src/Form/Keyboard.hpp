/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

/**
 * The PanelControl class implements the simplest form of a ContainerControl
 */
class KeyboardControl : public ContainerWindow {
public:
  typedef void (*OnCharacterCallback_t)(TCHAR key);

protected:
  enum {
    MAX_BUTTONS = 40,
  };

  const DialogLook &look;

  unsigned num_buttons;
  ButtonWindow buttons[MAX_BUTTONS];
  TCHAR button_values[MAX_BUTTONS];

public:
  /**
   * Constructor of the KeyboardControl class
   * @param parent the parent window
   * @param x x-Coordinate of the Control
   * @param y y-Coordinate of the Control
   * @param width Width of the Control
   * @param height Height of the Control
   */
  KeyboardControl(ContainerWindow &parent, const DialogLook &look,
                  PixelScalar x, PixelScalar y,
                  UPixelScalar width, UPixelScalar height,
                  OnCharacterCallback_t function,
                  const WindowStyle _style = WindowStyle());

  /**
   * Show only the buttons representing the specified character list.
   */
  void SetAllowedCharacters(const TCHAR *allowed);

  void SetOnCharacterCallback(OnCharacterCallback_t Function) {
    mOnCharacter = Function;
  }

protected:
  virtual void on_paint(Canvas &canvas);
  virtual bool on_command(unsigned id, unsigned code);
  virtual bool on_resize(UPixelScalar width, UPixelScalar height);

private:
  UPixelScalar button_width;
  UPixelScalar button_height;

  ButtonWindow *get_button(TCHAR ch);

  void move_button(TCHAR ch, PixelScalar left, PixelScalar top);
  void resize_button(TCHAR ch, UPixelScalar width, UPixelScalar height);
  void resize_buttons();
  void set_buttons_size();
  void move_buttons_to_row(const TCHAR* buttons, int row,
                           PixelScalar offset_left = 0);
  void move_buttons();

  bool is_landscape();

  void add_button(const TCHAR* caption);

  OnCharacterCallback_t mOnCharacter;
};

#endif
