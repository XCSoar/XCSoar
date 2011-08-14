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

#include "Form/CheckBox.hpp"
#include "Look/DialogLook.hpp"

CheckBoxControl::CheckBoxControl(ContainerWindow &parent,
                                 const DialogLook &look,
                                 const TCHAR *Caption,
                                 int X, int Y, unsigned Width, unsigned Height,
                                 const CheckBoxStyle style,
                                 ClickNotifyCallback_t Function) :
  mOnClickNotify(Function)
{
  set(parent, Caption, X, Y, Width, Height, style);
  set_font(*look.text_font);
}

bool
CheckBoxControl::on_clicked()
{
  // Call the OnClick function
  if (mOnClickNotify != NULL) {
    mOnClickNotify(*this);
    return true;
  }

  return false;
}

#ifdef _WIN32_WCE

bool
CheckBoxControl::on_key_check(unsigned key_code) const
{
  switch (key_code) {
  case VK_RETURN:
    return true;

  default:
    return false;
  }
}

bool
CheckBoxControl::on_key_down(unsigned key_code)
{
  switch (key_code) {
#ifdef GNAV
  // JMW added this to make data entry easier
  case VK_F4:
#endif
  case VK_RETURN:
    set_checked(!get_checked());
    return on_clicked();
  }

  return CheckBox::on_key_down(key_code);
}

#endif /* _WIN32_WCE */
