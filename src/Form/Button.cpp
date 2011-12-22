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

#include "Form/Button.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/Key.h"
#include "Asset.hpp"

WndButton::WndButton(ContainerWindow &parent, const DialogLook &_look,
                     const TCHAR *Caption, const PixelRect &rc,
                     const ButtonWindowStyle style,
                     ClickNotifyCallback _click_callback,
                     LeftRightNotifyCallback _left_callback,
                     LeftRightNotifyCallback _right_callback)
  :look(_look),
   click_callback(_click_callback),
   left_callback(_left_callback),
   right_callback(_right_callback)
{
  set(parent, Caption, rc, style);
  set_font(*look.button_font);
}

bool
WndButton::on_clicked()
{
  // Call the OnClick function
  if (click_callback != NULL) {
    click_callback(*this);
    return true;
  }

  return ButtonWindow::on_clicked();
}

bool
WndButton::on_left()
{
  // call on Left key function
  if (left_callback != NULL) {
    left_callback(*this);
    return true;
  }
  return false;
}

bool
WndButton::on_right()
{
  // call on Left key function
  if (right_callback != NULL) {
    right_callback(*this);
    return true;
  }
  return false;
}

bool
WndButton::on_key_check(unsigned key_code) const
{
  switch (key_code) {
  case VK_LEFT:
    return left_callback != NULL;

  case VK_RIGHT:
    return right_callback != NULL;

  default:
    return ButtonWindow::on_key_check(key_code);
  }
}

bool
WndButton::on_key_down(unsigned key_code)
{
  switch (key_code) {
  case VK_LEFT:
    return on_left();

  case VK_RIGHT:
    return on_right();
  }

  return ButtonWindow::on_key_down(key_code);
}
