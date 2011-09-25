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
    const TCHAR *Caption, int X, int Y, int Width, int Height,
                     const ButtonWindowStyle style,
    ClickNotifyCallback_t Function,
    LeftRightNotifyCallback_t LeftFunction,
    LeftRightNotifyCallback_t RightFunction)
  :look(_look),
    mOnClickNotify(Function),
    mOnLeftNotify(LeftFunction),
    mOnRightNotify(RightFunction)
{
  set(parent, Caption, X, Y, Width, Height, style);
  set_font(*look.button_font);
}

bool
WndButton::on_clicked()
{
  // Call the OnClick function
  if (mOnClickNotify != NULL) {
    mOnClickNotify(*this);
    return true;
  }

  return false;
}

bool
WndButton::on_left()
{
  // call on Left key function
  if (mOnLeftNotify != NULL) {
    mOnLeftNotify(*this);
    return true;
  }
  return false;
}

bool
WndButton::on_right()
{
  // call on Left key function
  if (mOnRightNotify != NULL) {
    mOnRightNotify(*this);
    return true;
  }
  return false;
}

bool
WndButton::on_key_check(unsigned key_code) const
{
  switch (key_code) {
  case VK_LEFT:
    return mOnLeftNotify != NULL;

  case VK_RIGHT:
    return mOnRightNotify != NULL;

  default:
    return false;
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


