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

#include "Form/CheckBox.hpp"
#include "Form/ActionListener.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/Key.h"

CheckBoxControl::CheckBoxControl(ContainerWindow &parent,
                                 const DialogLook &look,
                                 tstring::const_pointer caption,
                                 const PixelRect &rc,
                                 const CheckBoxStyle style,
                                 ClickNotifyCallback _click_notify_callback)
  :listener(nullptr), click_notify_callback(_click_notify_callback)
{
  CheckBox::Create(parent, caption, rc, style);
  SetFont(*look.text_font);
}

void
CheckBoxControl::Create(ContainerWindow &parent, const DialogLook &look,
                        tstring::const_pointer caption,
                        const PixelRect &rc,
                        const CheckBoxStyle style,
                        ActionListener &_listener, int _id)
{
  listener = &_listener;
#ifdef USE_GDI
  id = _id;
  CheckBox::Create(parent, caption, rc, style);
#else
  /* our custom SDL/OpenGL button doesn't need this hack */
  CheckBox::Create(parent, caption, _id, rc, style);
#endif
  SetFont(*look.text_font);
}

bool
CheckBoxControl::OnClicked()
{
  if (listener != nullptr) {
#ifndef USE_GDI
    unsigned id = GetID();
#endif

    listener->OnAction(id);
    return true;
  }

  // Call the OnClick function
  if (click_notify_callback != NULL) {
    click_notify_callback(*this);
    return true;
  }

  return false;
}

#ifdef _WIN32_WCE

bool
CheckBoxControl::OnKeyCheck(unsigned key_code) const
{
  switch (key_code) {
  case KEY_RETURN:
    return true;

  default:
    return false;
  }
}

bool
CheckBoxControl::OnKeyDown(unsigned key_code)
{
  switch (key_code) {
#ifdef GNAV
  // JMW added this to make data entry easier
  case KEY_APP4:
#endif
  case KEY_RETURN:
    SetState(!GetState());
    return OnClicked();
  }

  return CheckBox::OnKeyDown(key_code);
}

#endif /* _WIN32_WCE */
